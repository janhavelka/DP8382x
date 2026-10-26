# 28 - Validate packages, metadata and ownership

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-27.

## Fresh audit and owned work

Inspect TCA9548A/SCD41 clean package consumers and archive safety tests, TMP1x2/MB85RC
version tools, current repo license/remote and actual maintainers.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Finalize package metadata for the portable library and selected ESP-IDF adapter.
Preserve one release version source and deterministic generation. Express only
tested/justified framework/SDK constraints and actual supported part scope. No invented
URL, maintainer, CODEOWNERS account, registry version or publication.

Create a bounded package checker with explicit allowed contents, path/symlink/duplicate
protections and required license notices. Exclude docs/reference vendor payloads and
extracted text, firmware/build artifacts, test fixtures/HIL logs and private local paths
from firmware/library distribution as appropriate. Preserve notices for source actually
reused.

Build packages locally, extract into a clean temporary directory and compile/link an
ordinary consumer and real SDK example from exported contents. Do not fall back to
source-tree files, Git/sibling includes, generated files from a previous build or
globally installed headers. Include necessary adapter sources and supported public
paths.

Test source archives without Git and tests-OFF consumption without Python/Doxygen. Keep
example build identity unknown when its source provenance is unavailable rather than
adopting the parent checkout's SHA. Document dependency resolution and minimum
toolchains. Update CONTRIBUTING and ownership guidance with facts.

## Acceptance and handoff

Run positive package consumption and negative archive/member/content tests, version-sync
check and generated-file drift checks. Inspect package payload and licenses. Native/SDK
packages must link from extraction. Publication and tags are outside this stage.

Expected output: Reproducible consumable packages and accurate project metadata. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
