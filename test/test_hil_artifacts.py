"""Test durable runner evidence using disposable directories and injected file failures."""

import base64
import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from tools import dp8382x_hil_artifacts as artifacts


def case(verdict="PASS", *, executed=True, required=False, restoration="NOT RUN"):
    return {"id": "identity", "verdict": verdict, "executed": executed,
            "primary": {"code": "OK", "detail": 0} if executed else None,
            "restoration": {"required": required, "verdict": restoration}, "message": "Observed identity"}


def run_data(**changes):
    target = {"target": "host", "backend": "fake_mdio", "variant": "DP83826I", "interface": "RMII", "address": 1}
    return {"suite": "smoke", "fixture": "host", "started_utc": "2026-11-20T01:02:03Z",
            "ended_utc": "2026-11-20T01:02:04+00:00", "complete": True,
            "commands": ["session audit", "version"], "cases": [case()],
            "identity": {**target, "boot": 7, "session_req": 1, "firmware_strength": "exact"},
            "expectations": target, "runner": {"revision": "a" * 40, "tracked_tree": "clean"},
            "stderr": "", "issues": [], **changes}


class FaultyStream:
    def __init__(self, stream, fault):
        self.stream = stream
        self.fault = fault

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.stream.close()
        if self.fault == "close":
            raise OSError("simulated close failure")

    def write(self, content):
        if self.fault == "write":
            raise OSError("simulated write failure")
        if self.fault == "short":
            content = content[:-1]
        return self.stream.write(content)

    def flush(self):
        if self.fault == "flush":
            raise OSError("simulated flush failure")
        self.stream.flush()

    def fileno(self):
        return self.stream.fileno()


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)

    def open_run(self, run="run", limit=artifacts.MAX_TRANSCRIPT_BYTES):
        files = artifacts.EvidenceFiles(self.directory, run, limit)
        self.addCleanup(files.abort)
        return files

    def assert_pair(self, name="run"):
        self.assertEqual(sorted(path.name for path in self.directory.iterdir()), [name + ".json", name + ".txt"])

    def test_placeholder_first_and_actual_hash_after_close(self):
        files = self.open_run()
        empty, content = artifacts.verify_artifacts(files.path)
        self.assertEqual((empty["verdict"], empty["cases"][0]["verdict"], content), ("INCONCLUSIVE", "NOT RUN", b""))
        payload = b"prompt\r\n\x00\xff@DP8382X\n"
        self.assertEqual(files.write(payload[:8]), 8)
        self.assertEqual(files.write(payload[8:]), len(payload) - 8)
        stream, publish = files._stream, artifacts._publish

        def check_order(*args):
            self.assertTrue(stream.closed)
            self.assertEqual(files.transcript_path.read_bytes(), payload)
            return publish(*args)

        with mock.patch.object(artifacts, "_publish", side_effect=check_order):
            final = files.finish(run_data())
        self.assertIsNone(files._stream)
        self.assertEqual(final["verdict"], "PASS")
        self.assertEqual(final["transcript"], {"length": len(payload), "truncated": False,
                                                "sha256": hashlib.sha256(payload).hexdigest()})
        self.assertEqual(artifacts.verify_artifacts(files.path), (final, payload))
        self.assert_pair()
        for action, argument in ((files.write, b"late"), (files.finish, run_data())):
            with self.assertRaises(artifacts.EvidenceError):
                action(argument)

    def test_exclusive_reservation_does_not_touch_collisions(self):
        for suffix in ("json", "txt"):
            with self.subTest(suffix=suffix):
                prior = self.directory / f"run.{suffix}"
                prior.write_bytes(b"older run")
                with self.assertRaisesRegex(artifacts.EvidenceError, "Cannot reserve"):
                    self.open_run()
                self.assertEqual([self.directory / prior.name], list(self.directory.iterdir()))
                self.assertEqual(prior.read_bytes(), b"older run")
                prior.unlink()
        files = self.open_run()
        original = files.path.read_bytes()
        with self.assertRaises(artifacts.EvidenceError):
            self.open_run()
        self.assertEqual(files.path.read_bytes(), original)
        self.assert_pair()

    def test_truncated_prefix_and_upstream_limit_cannot_pass(self):
        files = self.open_run("prefix", 4)
        self.assertEqual(files.write(b"\xffbc"), 3)
        self.assertEqual(files.write(b"def"), 1)
        self.assertEqual(files.write(b"more"), 0)
        self.assertTrue(files.truncated)
        self.assertEqual(files.finish(run_data())["verdict"], "INCONCLUSIVE")
        manifest, content = artifacts.verify_artifacts(files.path)
        self.assertEqual((manifest["transcript"]["truncated"], content), (True, b"\xffbcd"))

        source = self.open_run("source")
        source.write(b"stdout prefix")
        source.mark_truncated()
        self.assertEqual(source.finish(run_data())["verdict"], "INCONCLUSIVE")

    def test_abort_retains_bytes_and_placeholder(self):
        files = self.open_run()
        placeholder = files.path.read_bytes()
        files.write(b"some received bytes")
        files.abort()
        files.abort()
        self.assertEqual(files.transcript_path.read_bytes(), b"some received bytes")
        self.assertEqual(files.path.read_bytes(), placeholder)
        with self.assertRaisesRegex(artifacts.EvidenceError, "does not match"):
            artifacts.verify_artifacts(files.path)
        self.assert_pair()

    def test_failure_dominates_and_incomplete_cannot_pass(self):
        data = (
            (run_data(cases=[]), "INCONCLUSIVE"),
            (run_data(cases=[case("NOT RUN", executed=False)]), "INCONCLUSIVE"),
            (run_data(cases=[case(required=True)]), "INCONCLUSIVE"),
            (run_data(cases=[case(required=True, restoration="FAIL")]), "FAIL"),
            (run_data(cases=[case("FAIL", required=True, restoration="PASS")]), "FAIL"),
            (run_data(complete=False, cases=[case("FAIL")]), "FAIL"),
            (run_data(complete=False), "INCONCLUSIVE"),
            (run_data(complete=False, issues=["terminal record missing"]), "INCONCLUSIVE"),
            (run_data(commands=[]), "INCONCLUSIVE"),
            (run_data(expectations={}), "INCONCLUSIVE"),
        )
        for index, (record, expected) in enumerate(data):
            with self.subTest(record=record):
                files = self.open_run(f"run-{index}")
                files.write(b"received")
                self.assertEqual(files.finish(record)["verdict"], expected)
                self.assertEqual(artifacts.verify_artifacts(files.path)[0]["verdict"], expected)
        files = self.open_run("none")
        self.assertEqual(files.finish(run_data())["verdict"], "INCONCLUSIVE")

    def test_write_sync_and_close_failures_preserve_the_placeholder(self):
        for fault in ("write", "short", "flush", "close"):
            with self.subTest(fault=fault):
                files = self.open_run(fault)
                previous = files.path.read_bytes()
                file = files._stream
                files._stream = FaultyStream(file, fault)
                if fault in ("write", "short"):
                    with self.assertRaisesRegex(artifacts.EvidenceError, "Cannot write transcript"):
                        files.write(b"received")
                else:
                    files.write(b"received")
                with self.assertRaises(artifacts.EvidenceError):
                    files.finish(run_data())
                self.assertTrue(file.closed)
                self.assertEqual(previous, files.path.read_bytes())
                self.assertTrue(files.transcript_path.is_file())

    def test_read_hash_temp_sync_and_replace_failures_preserve_the_placeholder(self):
        faults = (
            mock.patch.object(artifacts, "_bounded_read", side_effect=OSError("cannot reopen")),
            mock.patch.object(artifacts.hashlib, "sha256", side_effect=OSError("cannot hash")),
            mock.patch.object(artifacts.os, "fsync", side_effect=[None, OSError("simulated manifest sync failure")]),
            mock.patch.object(artifacts.os, "replace", side_effect=OSError("cannot replace")),
        )
        for index, fault in enumerate(faults):
            with self.subTest(index=index):
                files = self.open_run(f"fail-{index}")
                previous = files.path.read_bytes()
                files.write(b"received")
                with fault, self.assertRaises(artifacts.EvidenceError):
                    files.finish(run_data())
                self.assertEqual(previous, files.path.read_bytes())
                self.assertEqual(files.transcript_path.read_bytes(), b"received")
                self.assertEqual(list(self.directory.glob("*.tmp")), [])

    def test_same_length_data_change_preserves_the_placeholder(self):
        files = self.open_run()
        previous = files.path.read_bytes()
        files.write(b"capture")
        with mock.patch.object(artifacts, "_bounded_read", return_value=b"changed"):
            with self.assertRaisesRegex(artifacts.EvidenceError, "bytes changed"):
                files.finish(run_data())
        self.assertEqual(files.path.read_bytes(), previous)

    def test_writer_will_not_publish_ambiguous_or_overflowed_json(self):
        for index, extra in enumerate(({1: 0, "1": 1}, {"invalid": 1 << 100}, {"invalid": float("nan")})):
            with self.subTest(extra=extra):
                files = self.open_run(f"invalid-{index}")
                original = files.path.read_bytes()
                files.write(b"captured stdout")
                invalid = run_data()
                invalid["runner"].update(extra)
                with self.assertRaises(artifacts.EvidenceError):
                    files.finish(invalid)
                self.assertEqual(files.path.read_bytes(), original)

    def test_stderr_round_trips_with_canonical_bounded_base64(self):
        files = self.open_run("stderr", 16)
        content = b"\xff\x00\r\n"
        files.write(b"stdout")
        encoded = base64.b64encode(content).decode("ascii")
        final = files.finish(run_data(stderr=encoded))
        self.assertEqual(base64.b64decode(artifacts.verify_artifacts(files.path)[0]["stderr"]), content)
        for invalid in ("//==", "\n" + encoded, "invalid?", False):
            files.path.write_text(json.dumps({**final, "stderr": invalid}), encoding="utf-8")
            with self.assertRaisesRegex(artifacts.EvidenceError, "stderr"):
                artifacts.verify_artifacts(files.path)

        over = self.open_run("limit", 8)
        placeholder = over.path.read_bytes()
        over.write(b"stdout")
        with self.assertRaisesRegex(artifacts.EvidenceError, "Combined"):
            over.finish(run_data(stderr=encoded))
        self.assertEqual(placeholder, over.path.read_bytes())

    def test_verify_rejects_tampered_manifest_and_transcript(self):
        files = self.open_run()
        files.write(b"run transcript")
        original = files.finish(run_data())
        fields = (("schema", True), ("scope", "physical"), ("run", "another"), ("verdict", "FAIL"),
                  ("complete", 1), ("started_utc", "not-time"), ("ended_utc", "2026-11-20T01:02:02+01:00"))
        for key, invalid in fields:
            with self.subTest(key=key):
                files.path.write_text(json.dumps({**original, key: invalid}), encoding="utf-8")
                with self.assertRaises(artifacts.EvidenceError):
                    artifacts.verify_artifacts(files.path)
        for section, key, invalid in (("transcript", "length", True), ("transcript", "truncated", 0),
                                      ("identity", "boot", True), ("identity", "session_req", 1 << 64),
                                      ("expectations", "address", True)):
            with self.subTest(section=section, key=key):
                data = copy.deepcopy(original)
                data[section][key] = invalid
                files.path.write_text(json.dumps(data), encoding="utf-8")
                with self.assertRaises(artifacts.EvidenceError):
                    artifacts.verify_artifacts(files.path)
        for primary in ({"code": "OK", "detail": False}, {"code": "IN_PROGRESS", "detail": 0},
                        {"code": "MDIO_ERROR", "detail": 1 << 31}):
            data = copy.deepcopy(original)
            data["cases"][0]["primary"] = primary
            files.path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(artifacts.EvidenceError, "terminal status"):
                artifacts.verify_artifacts(files.path)

        raw = json.dumps(original)
        for invalid in (raw.replace('"boot": 7', '"boot": 7, "boot": 7'),
                        raw.replace('"tracked_tree": "clean"', '"tracked_tree": "clean", "extra": 1e999')):
            files.path.write_text(invalid, encoding="utf-8")
            with self.assertRaises(artifacts.EvidenceError):
                artifacts.verify_artifacts(files.path)
        files.path.write_text(raw, encoding="utf-8")
        for invalid in (b"Run transcript", b"run transcript+", b"x" * (artifacts.MAX_TRANSCRIPT_BYTES + 1)):
            files.transcript_path.write_bytes(invalid)
            with self.assertRaises(artifacts.EvidenceError):
                artifacts.verify_artifacts(files.path)
        files.path.write_bytes(b" " * (artifacts.MAX_MANIFEST_BYTES + 1))
        with self.assertRaisesRegex(artifacts.EvidenceError, "byte limit"):
            artifacts.verify_artifacts(files.path)


if __name__ == "__main__":
    unittest.main()
