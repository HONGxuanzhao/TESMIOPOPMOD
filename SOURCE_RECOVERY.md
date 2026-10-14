# TESMIOPOPMOD: source recovery and validation

Recovery date: 2026-10-15 (Asia/Shanghai).

Original C++ files and their later edits were recovered from local development records. The source was not generated from decompiler pseudocode. Private conversation logs, credentials, local build products and backup folders are excluded from this repository.

The retained TESMIO API 4 header is a minimal ABI declaration. Build scripts were adapted to keep output in `build/`; the POPMOD header include path was made independent of the checkout directory name. No license for the recovered author code has been assigned by this upload.

## Verification

- Successfully rebuilt with Microsoft Visual C++ 14.51.36231 for Windows x64.
- The complete `.text` machine-code section is byte-for-byte identical to the published DLL.
- The entire rebuilt DLL is byte-for-byte identical after zeroing only the COFF header timestamp and IMAGE_DEBUG_DIRECTORY timestamp fields.
- The original published DLL remains in `plugins/`.
- No fresh live-game session was run for this publication.

Original DLL SHA-256: `49a62fef4ea5b95ab438b7252b3e3c1151a826ae2f56d29158fdbdadf3b74d90`

Rebuilt DLL SHA-256: `bb9b1e694a0d4e82ea8a7a173ff80b361ea1fb8507d6fc2bef73da40f9963dd6`

SHA-256 after timestamp normalization: `c256d119e9b376c52216bf0251f6080fd4d8b0991d7de6153de2ef0c68df357d`

归档源码从本机原始开发记录与修改记录恢复。5 个工程均重新编译成功；此 MOD 重建 DLL 的完整机器码与原发布 DLL 相同，整个文件仅有 PE 时间戳字段不同。发布包保留原 DLL；这次上传没有进行新的游戏运行测试。
