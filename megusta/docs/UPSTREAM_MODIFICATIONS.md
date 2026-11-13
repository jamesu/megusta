# Upstream modifications

This records library changes and their comparison baselines. Per-entry test
notes describe the scope at the time of a change, rather than current project
completion. Current build/runtime guides are linked from the root README.

## CPython 1.5.2 — aggregate vendor patch

The active interpreter sources are in `vendor/Python-1.5.2/` and compile
through `Megusta/CPython.cmake`, included by the thin project CMake wrapper.
`patches/python/0001-megusta.patch` records 13 changed upstream source files
and six added build/configuration files against the retained CPython 1.5.2
baseline: pointer conversion/hash/representation adaptations, Windows time
and registry API compatibility, the POSIX `getline` name collision and the
browser quit/reload behavior. Applying the patch to the baseline reproduces
all 1,433 vendor files byte-for-byte, excluding filesystem metadata.

To recapture, or check without overwriting the recorded patch:

```sh
python3 tools/codegen/capture_python_patch.py /path/to/Python-1.5.2
python3 tools/codegen/capture_python_patch.py /path/to/Python-1.5.2 --check
```

The baseline is an explicit tool input, not a compilation dependency.
`Megusta/` contains the actual compiled configuration and support files:
`config.h`, `builtin_config.c`, `interrupt_stubs.c`, `largeint.h`,
`runtime_setup.h` and `CPython.cmake`. The patch therefore includes the platform
and pointer-width definitions, embedded module table, disabled interrupt/thread
policy, runtime initialization flags and compiler/source selection. There is
no separate unpatched configuration copy under the application source tree.

The active image-library trees are `vendor/libpng-1.0.8/` and
`vendor/zlib-1.1.3/`. Their existing macOS fixes are retained: `pngconf.h`
does not select classic Mac `<fp.h>` on modern Apple targets, and `zconf.h`
keeps the `Byte` typedef available there.

## Emscripten browser target adaptations

The wasm32 browser target keeps the same game sources and uses Emscripten SDL2
plus legacy fixed-function GL emulation. `MappedResourceIndex` and
`EngineDirectoryIterator` use the Emscripten virtual filesystem instead of
Win32 file mappings and `FindFirstFile`; Win32 retains those original OS paths.
PLIB SSG/PUI use the SDL-owned WebGL context and SDL tick clock in browser mode.
The browser CPython configuration selects the POSIX `getpath` unit and keeps
threading and dynamic module loading disabled. CPython's
private `fileobject_getline` helper is renamed because modern POSIX libc also
declares `getline` with an incompatible signature. These adaptations are
browser-only and do not modify the Win32 target's selected source behavior.
The active browser renderer links vendored gl4es for legacy OpenGL behavior on
WebGL. The earlier project-local legacy-GL shim is not the active renderer.
The browser inittab omits CPython's `_socket` module because the original
Windows socket API is not a browser socket transport; offline gameplay remains
available while online networking needs a WebSocket service adapter.

## CPython 1.5.2 — x64 pointer-width adaptations

Files: `vendor/Python-1.5.2/Megusta/config.h`, plus the listed files under
`vendor/Python-1.5.2/`. These changes activate only when `_WIN64`, `_M_X64`
or `_M_AMD64` is defined. The old Windows configuration describes x86 and
Alpha, so it reported 4-byte pointers on Windows x64 even though CPython's
`PyLong_FromVoidPtr` and `PyLong_AsVoidPtr` already contain a wider-pointer
path. The local config now reports 8-byte pointers while retaining Windows'
4-byte `long` size.

- `Python/bltinmodule.c`: `id()` uses `PyLong_FromVoidPtr` on x64.
- `Modules/arraymodule.c`: `array.buffer_info()` returns its address as a
  full-width Python integer on x64.
- `Modules/cPickle.c`: pointer-based memo keys use full-width Python integers,
  preventing distinct object addresses with the same low 32 bits from
  colliding.
- `Objects/{bufferobject,classobject,fileobject,funcobject,methodobject,object}.c`
  and `Python/compile.c`: diagnostic repr strings use `%p` on x64 rather than
  printing a truncated `long` address.
- `Objects/{classobject,funcobject,methodobject,object}.c`: pointer-derived
  hashes fold the upper pointer bits into the 32-bit `long` hash result. Hash
  callbacks still return the CPython 1.5 `long` type; x86 keeps its prior
  address-derived result.

All these source changes are guarded by the x64 size comparison or use the
config-selected formatting/hash macros, so Win32 behavior stays on its prior
path. These changes preserve full-width pointer conversion while retaining
the interpreter's expected integer/hash interfaces.

## MG-PLIB-094 — preserve SSG token-buffer bounds check on x64

File: `vendor/plib/src/ssg/ssgParser.cxx`. The original check converted both
buffer pointers to `long` before subtracting them. Windows x64 keeps `long` at
32 bits, so that pointer conversion is invalid and prevents a 64-bit build.
Subtracting the pointers directly measures the same distance within their
shared token-buffer array and preserves the existing 4096-byte assertion.
This is a portability correction; it does not alter the intended bound.

## MG-PLIB-092 — const-correct chat terminator lookup

`vendor/plib/src/net/netChat.cxx` retains the `const char*` returned by the
C++ CRT `strstr` overload for a const input buffer. The code only reads the
pointer and subtracts it from the original buffer; behavior is unchanged.
This permits compilation with the Microsoft C++ headers and ClangCL.

## MG-PLIB-093 — MSVC-compatible SSG vector and character lookup types

`ssgLoadASE.cxx` selects between two `sgVec3` array members. MSVC decays the
arrays in the conditional expression, so a reference-to-array cannot bind to
the result. The parser only indexes the selected vector; it now keeps the
selected scalar buffer as `float*`, preserving the same three-component writes.
`ssgParser.cxx`'s `mystrchr` helper only tests its result for null. Its return
type is now `const char*`, matching the Microsoft CRT overload for a const input
string. These changes preserve runtime behavior and allow the selected SSG
sources to compile with the Windows SDK headers.

This is the single authoritative change log. The complete PLIB 1.3.2 distribution
in `upstream/plib-1.3.2/` is the pristine input. The working distribution is
`vendor/plib/`; the build directly compiles its original source files. All local
changes are represented by `patches/plib/0001-megusta.patch`, with comment IDs at
the changed regions. Its hash and every output file hash are recorded in
`vendor/plib/megusta-patches.json`. Preparation verifies the pristine input against
`upstream/manifest.json`; `--verify-rebuild` verifies clean patch reproduction.

PLIB 1.3.2 is an integration baseline, not an established exact linked release.
The binary includes older serialization and game adaptations. Source counterparts
and current per-function review status remain in the global index. Compiling an
unchanged upstream source file does not approve every function as equivalent.

## MG-PLIB-091 — place vertex primitive type in `ssgVtxTable`

File: `vendor/plib/src/ssg/ssg.h`.

The executable's `ssgLeaf_ctor` at `0047c300` initializes the four vertex,
normal, texcoord and color counts at offsets `+0x5c..+0x68`, followed by its
draw callbacks and display-list field. `ssgVtxTable_ctor_at_0047ee30` places
the bounds at `+0x78`, the primitive type at `+0x90`, and the four owned array
pointers at `+0x94..+0xa0`; `ssgVtxTable_dtor` at `0047f030` releases those
arrays. The vendored header had instead put `gltype` in `ssgLeaf`, shifting the
count/callback/bounds members and leaving the derived primitive field absent.
Move `gltype` after `ssgVtxTable::bbox` so member accesses and destruction match
the executable layout on 32-bit builds. The four array-pointer offsets remain
unchanged, which is why scene loading alone did not expose this mismatch.
`ssgVTable` uses the same derived-field arrangement in the current source as a
compatibility inference; its corresponding executable offsets have not yet
been independently confirmed.

This is a binary-evidenced PLIB layout correction, not an inference from the
crash alone. A controlled four-player skip run still faults during teardown,
after releasing a 60-entry state selector's state objects and while freeing
its step-pointer array. Thus this layout correction is independently supported
but is not the fix for that crash. The exact restart action remains untested.

## MG-PLIB-001 — evidenced base/branch behavior and defined list iteration

Files: `src/ssg/ssgBase.cxx`, `ssgBranch.cxx`, `ssgList.cxx` in the vendor tree.

- At binary 00466620, base copy always checks equal types, assigns the source
  type, fetches spare, then resets reference count; it has no stock self-copy
  early return. The vendor body follows that order and retains the existing
  release-before-acquire userdata behavior. Type mismatch terminates even in a
  build with assertions disabled. The diagnostic is a local service adaptation;
  its exact text is not claimed to match the original fatal backend.
- Branch addKid 0045b290 and removeKid 0045b2c0 ignore a null child. The vendor
  methods return before changing child lists, parent lists or dirty flags.
- The list constructor initializes `next` to zero for defined native iteration.
  This is explicitly a portability change: 0048e280 does not initialize that
  field. It is not attributed to the original developers.

Evidence: `manifests/evidence/plib_binary_changes.json` retains the relevant
decompiler bodies, original code hashes, observations and canonical export hash.
The static snapshot is reproducible through the recovery-side evidence exporter.
SSG tests exercise null children, lookup, cloning, spare/type/refcount state,
shared parent ownership and teardown. Original-process comparisons are pending.

## MG-PLIB-002 — optional OpenGL header dependency

File: `src/util/ul.h`. GL/GLU imports remain available for the actual SSG build;
`MEGUSTA_NO_OPENGL` can exclude them for a non-rendering SG/UL target. No graphics
functions are stubbed. This is target portability, not a binary-observed change.

## MG-PLIB-003 — numerical bindings and library compiler profile

Files: `src/sg/sg.h`, `src/sg/sg.cxx`, added `src/sg/megusta_math.h`.
Math operations explicitly select double-argument/double-result sqrt, sin, cos,
tan, fabs, asin, acos, atan, atan2 and pow entry points instead of modern C++ float
overloads. SG uses `-fno-fast-math -ffp-contract=off -ffp-eval-method=double`.
UL and SSG retain normal expression evaluation with fast-math and contraction
disabled. The exact command lines are in the current compile database.

This remains a provisional approximation of wider x87 intermediates. It does not
establish the original compiler flags, control word, rounding at every store,
80-bit temporaries or transcendental equivalence. Any flag change needs numerical
comparison evidence. No reassociation/FMA optimization is justified by successful
scene fixture agreement alone. The SG expected-value tests cover matrix order,
normal/cross calculations, volume extension and frustum classification/warnings.

## MG-PLIB-004 — modern C++ and filesystem portability

- `src/util/ul.h` defines NOMINMAX before importing Windows headers.
- `src/ssg/ssgBase.cxx` releases name buffers with `delete[]`, matching their
  `new[]` allocation. This removes a language-level allocation/deallocation
  mismatch; it is a portability fix, not a newly discovered game change.
- `src/ssg/ssg.h` likewise pairs the texture filename allocation with `delete[]`.
  `ssgStateSelector.cxx` uses an `ssgSimpleState*` for selected-state virtual
  delegation instead of downcasting every child to `ssgStateSelector*`. This
  retains the virtual calls and self guards while removing invalid derived casts.
- `src/ssg/ssgLoadPNG.cxx` uses the distributed `GL/glpng.h` header spelling.

## MG-PLIB-005 — selected source/loader dependency closure

Files: `src/ssg/ssg.cxx`, `src/ssg/ssgconf.h`. Model registration is limited to
SSG and ASE rather than pulling unselected stock format implementations into the
link. PNG support is enabled against the already retained glpng/libpng/zlib
versions; MDL support is disabled. The full distribution still contains the
unbuilt format sources. These are current build-scope choices, not a complete
reconstruction of the game's registration table. The game ASX reader is separate
engine code and has not been advertised as the stock ASE loader.

## MG-PLIB-006 — state constructor and care-mask semantics

`src/ssg/ssgSimpleState.cxx` disables alpha testing in its cached-state constructor,
matching 0046c1c0 (the binary uses game enum0x23, the native target uses
GL_ALPHA_TEST). Both constructors initialize `enables` backing storage to zero
before read/modify/write. The latter is a defined-behavior portability choice:
the original leaves ignored bits uninitialized; no new material defaults are
invented and the original all-don't-care mask is retained.

The care helpers in `src/ssg/ssg.h` use unsigned shifts and explicitly mask the
count to five bits, matching 0046d140/0046d160/0046d180 rather than invoking C++
undefined behavior for bit31 or out-of-range counts. The isolated original-code
oracle compares every byte count0..255, capability masks, translucency and
selector pointer/index results. Native tests additionally exercise cached-state
defaults, complete material copies, selected-child delegation, shared/deep clones
and shared texture ownership.

### Preserved selector edge cases

The original and stock source recurse on some methods when an invalid/empty
selection makes `getCurrentStep()` return the selector itself. Null slots also
make recursive cloning unsafe because `getStep()` can return the source selector.
These are recorded in the evidence snapshot rather than silently repaired.
Delegation/clone tests use valid populated slots; fallback tests compare pointer
identity without invoking recursive operations. Legacy type IDs and serialization
remain separate review work and are not established by these tests.

### Original comparison scope

The Win32 helper launches the hash-pinned original executable suspended, replaces
its remote entry point with a platform wait stub, then allows OS loader
initialization to finish. A test DLL invokes bounded functions at their original addresses. Version0
serialization redirects file/allocation/diagnostic services to the harness CRT;
the original serializer instructions execute. It never runs original game startup and
never changes the executable file. The isolated process is terminated afterward.
This avoids relocating fixed-address code or overwriting the host process heap.

4,437 bounded comparisons across 226 entry points pass, including 256 bitwise vector products and 256
bitwise matrix products. The measured test-thread x87 control word is0x027f
(53-bit precision, round to nearest). This supports MG-PLIB-003's profile for
these cases; it does not establish original application startup control-word
settings, all SG math, NaN behavior, or whole-game FPU parity. Current results,
DLL/executable hashes and source hashes are in the validation report. A stale
report does not promote functions in the global index.

## MG-PLIB-007 — NET message framing and buffer overflow

Compared against PLIB 1.3.2. Binary `00415df0` writes a network-order frame
length; `0046d850` reads it in network order. The distributed source used host
order. Both directions now use network order. Frames shorter than five bytes
unconditionally reject through UL_FATAL and abort, as in the executable.
`00415cf0` requires channel output to remain strictly below capacity and closes
on overflow; the underlying netBuffer append remains inclusive of capacity.
Tests cover golden bytes, float payload bits, fragmented/pipelined frames,
short-frame rejection and exact-capacity closure. These are integration checks,
not yet original-code differential comparisons.

## MG-PLIB-008 — NET channel and monitor callbacks

Compared against PLIB 1.3.2. `00467470`/`004674a0` invoke a virtual hook after
handle adoption/open; `handleOpen` is an analyst name for that hook.
`00467640` resets channel flags even when already closed, while calling the
close callback only once. Monitor `00466f70` omits the inherited buffer reset;
`00467200` treats only character 8 as backspace, ignoring DEL. Localhost TCP
checks cover open/adoption callbacks, connection completion, data and EOF.
The native Winsock select adapter implements the recovered descriptor merging,
direction counts and list filtering (MG-PLIB-015); its platform-specific error
edge cases remain pending. Room protocol units were subsequently enabled and
adapted in MG-PLIB-010/013/048.

## MG-PLIB-009 — native socket error preservation

Compared against PLIB 1.3.2. Binary `00467e20` inspects nonblocking errors
without clearing them. The native Winsock branch now preserves WSAGetLastError
and omits the stock diagnostic print. Tests check preservation for would-block
and connection-refused. Native Winsock error values are used rather than the
original compatibility layer's numeric errno constants.

## MG-PLIB-010 — room player and game record extensions

Compared against PLIB 1.3.2. Player `0046e000/020/1a0/300` and game
`0046e340/360/550/700` have an additional unsigned 32-bit protocol value.
Its semantic meaning remains unresolved; `protocolValue` is an analyst name.
Player wire order puts it after watching; game wire order puts it before the
in-progress/password flags. The player constructor leaves it untouched, whereas
the game constructor zeroes it. Copy keeps the destination network id.
Original-code comparisons cover constructors, nine wire/copy operations per
record pair across empty, short and maximum-length names. Binary-layout offsets
exist only in the oracle; the production classes use ordinary target layouts.

## MG-PLIB-011 — FNT-only PUI configuration and window service

Compared against PLIB 1.3.2. The executable's puFont methods `004654c0` through
`004655b0` have only the FNT path. `MEGUSTA_PUI_FNT` retains puFont as a class
while excluding GLUT and stock Win32 bitmap-font paths. CMake exposes that
configuration to clients. The null-font constructor initializes backing size
and slant for defined modern behavior (a portability decision).
`puSetNativeWindow(HWND)` explicitly supplies the target window; client size
comes from GetClientRect. Before attachment the explicit viewport size is used.
The context checker unconditionally succeeds, matching `00456460`.
Rendering still uses native OpenGL, and engine rendering/cursor/filter changes
are not yet reproduced or claimed equivalent.

## MG-PLIB-012 — PUI input ordering

Compared against PLIB 1.3.2. `00461770` deactivates another active widget before
checking Return/default status. `00461850` and `004653a0` omit puMoveToLast and
invoke callbacks before assigning the active widget. The ordinary widget and
button now follow that order. Logic tests observe callback-visible active state,
ordinary-key deactivation, and repeated toggles. Draw/placement paths remain
separate review work.

## MG-PLIB-013 — room client protocol and discovery

Compared against PLIB 1.3.2. The client constructor argument supplies a 16-bit
UDP discovery base port, while the login wire version is fixed at 0x22.
Client buffers are 4096/8192. The client owns a room copy and discovery socket;
enter/leave/destruction perform the evidenced lifecycle operations.
Wire reply/update opcodes are 210/211/212/213/215/216/217; tag218 delivers room
info with its sender address. Login success invokes the added virtual hook.
Incoming game/player updates insert at their transmitted ids, rather than the
first vacant slot. Added addAt rejects invalid ids instead of reproducing the
original out-of-bounds write; malformed-id behavior is a portability exception.
Room address construction accepts an additional option. That value remains
uninitialized in the empty constructor and is neither serialized nor copied.
Discovery binds local-host ports from signed-short base through base+24, uses
nonblocking receive, consumes at most one packet per poll, and releases on enter.
New hook/discovery names are analyst names. The native socket service replaces
the original portability backend. Local integration checks capture outbound
frames and cover login, sparse updates, master/start, create/join/watch and leave.
Discovery/network error edge cases and original client differential execution
remain pending; this is not a whole-client parity claim.

## MG-PLIB-014 — font glyph advance

Compared against PLIB 1.3.2. Original `0045bd20/0045bda0` set/get a ninth glyph
float: cursor advance distinct from the right geometry edge. TXF loader `0048f2c0`
obtains it from signed glyph.step divided by maximum height. Proportional
cursor movement and bounding-box width use advance. Glyph geometry retains the
right edge. Font APIs and the TXF call site include the extra argument/output.
Original-code differential cases exercise all nine fields, unsigned char255,
fixed/proportional pitch, sizes, positive/negative slant, case fallback, spaces,
multiline and FNT-only PUI metric conversion. Original multiline/negative-slant
quirks are retained rather than repaired.

## MG-PLIB-015 — socket poll descriptor selection

Compared against PLIB 1.3.2. `00467e40` merges read/write arrays into at most
256 distinct valid handles and filters them using `004dc680` native select
results. It preserves duplicate entries in output arrays but counts ready
directions only once per distinct handle. Stock select counted duplicates.
The vendor adapter now follows that merge/filter/count behavior, rejects
capacity overflow, and omits closed handles with a diagnostic. It retains the
original adapter's ignored select return. The native Winsock service remains
the platform provider; this does not claim original errno numeric identity.
Local tests cover duplicate readiness and closed-handle filtering.

## MG-PLIB-016 — explicit SG floating-point evaluation boundaries

Compared against PLIB 1.3.2 and original x87 instructions at `00457050`,
`004572c0`, `00457af0`, `00456b50` and `00457890`. Broader bitwise differential
checks exposed differences hidden by the initial multiplication-only checks.
The original spills distance inputs and distances to float while keeping new
sphere radius and growth ratio wide. Its squared-distance reduction uses z/x/y
order. Normalization retains the reciprocal until component writes. Coordinate
angles round to float before the math adapter, while selected cosine/product
intermediates remain wide. The vendor source makes these boundaries explicit,
rather than assuming a compiler flag reproduces register allocation. Current
oracle results determine which cases pass; this entry alone is not a parity
claim. The measured oracle precision remains 53 bits; full-game startup FPU
control and alternate precision modes remain unresolved.

## MG-PLIB-017 — compile out unsupported PUI placement paths

Compared against PLIB 1.3.2. `00461380/00461520` only implement above, below,
left and right label placement; other placement values leave bounds unchanged
and do not draw a label. Extended centered-placement branches and the stock
default fallthrough are removed from those methods. `004651d0` always centers
its button legend, without the stock left/right justification switch.
Original-code comparisons cover all nine placement values at multiple font
sizes/slants, with matching font metrics. Rendering call parity remains separate.

## MG-PLIB-018 — SSG base diagnostics

Compared against PLIB 1.3.2. `00466840` unconditionally prints reference count,
name and userdata. The vendor print method retains its callable source signature
but ignores how_much, removing the stock conditional diagnostic branches.
Static comparison establishes this formatting/control-flow change; no original
FILE pointer is passed across CRT implementations to test it.

## MG-PLIB-019 — adapted executable SSG version 0 serializer

Comparison base: PLIB 1.3.2. Version0 bodies are adapted from the executable's
canonical decompilation/disassembly; PLIB 1.3.1 was consulted as a legacy guide.
No source outside Megusta is included in its build. The working vendor files
and ordinary distribution patch contain the implementation.

The reader accepts magic 0x53534700/version 0 only. The stock1.3.2 version 1
reader/writer has been replaced. Supported entity wire tags are VtxTable4007,
VtxArrayc007, Branch13, Transform73, Selector113, Cutout813, TexTrans2013 and
Root413. State/texture tags are d,1d and20001. These are wire tags: runtime
classes retain the1.3.2 internal flags, with an explicit translation boundary.
This does not claim original numeric getType compatibility outside serialization.

Changes are applied together across Base, Branch, Leaf, SimpleList, VtxTable,
VtxArray, SimpleState, StateSelector and TexTrans. Entity/State/BaseTransform,
Selector, Cutout and Texture bodies retain matching payload operations. Keys
are assigned when the base name payload is reached and registered before name
reads; arrays serialize directly without independent object keys. The leaf's
additional word is named render_order_mode after its recovered role and is
initialized, copied in the evidenced order and serialized. Graphics tokens use
the 128-entry original translation table, including primitive, shading and
colour-material enums. Native pointers, vtables and compiler layouts are never
written into the wire stream.

| Record | Leading/trailing markers |
|---|---|
| Branch child | 12345678 / 23456789 |
| Leaf | 38b83 / 23731f |
| Array | 18a92 / f69b5 |
| Simple state | 3039 / 1e240 |

Markers are consumed without validation, matching the executable. Reference
installation and ref-count changes precede virtual state/texture/step loading,
as in the original. Unknown state/texture field tags warn and continue as null;
the texture fallback does not invoke the care-bit-changing setter.

Preserved original quirks:

- A branch backward reference invokes the referenced object's load again.
  Shared-entity writer bytes are compared; this broken reader path is retained
  from static evidence, not advertised as a successful shared-entity round trip.
- A leaf tag 1d constructs a simple state rather than a selector. Standalone
  selector-state payloads are supported and compared separately. The leaf quirk
  remains intentionally unrepaired.
- Recursive spare reset does not clear shared state spares. Subsequent writes
  can therefore emit stale references, as in the original. Comparisons include
  readback/resave behavior rather than promising every saved file reloads.

Native portability differences are explicit: files close on failure, replaced
storage is released, state-selector arrays start with null pointers, prior list
size controls cleanup, and invalid/null branch references fail instead of
reproducing null dereferences. These changes affect resource/undefined-error
behavior, not the compared valid wire records.

The isolated original-code harness redirects file/allocation/diagnostic service
boundaries, avoiding original CRT startup and mixing incompatible FILE layouts.
Its original code writes complete files from raw objects only at the binary
boundary. Production code uses named C++ classes. Comparisons cover all eight
entity tags, mesh/index arrays, material payloads, shared-state identity, selector
payloads with shared/null steps, texture metadata writes, ignored markers,
unsupported versions/magic/entity types and the unknown-state fallback. Texture
image loading and renderer/texture-registry parity are not claimed by these tests.

Current coverage is 4,437 comparisons across 226 original entry points; 27 checks
belong to this version 0 group. Other nested methods execute within those file
fixtures but are not inflated into extra individually compared entry points.
Full-game startup/FPU behavior remains outside this scope. All Wine inputs are
copied to C: inside `/tmp/mgm-wineprefix`; no other drives are mapped.

`recomp/export_megusta_ssg_v0_evidence.py` reproduces the body/ABI and token
snapshots under `manifests/evidence/`. Its optional --write-token-header action
regenerates the pinned private token header; it does not fetch source. Thirty
member load/save ABIs now explicitly use ECX self, Stack4 FILE* and RET4 in
Ghidra. The cdecl Float/UInt/Int writers' missing value arguments are restored.
The global index retains per-address providers and hash-gated bounded results.
## MG-PLIB-020 — restrict material setter to executable cases

Comparison base: **PLIB 1.3.2**, `src/ssg/ssg.h`,
`ssgSimpleState::setMaterial(GLenum, sgVec4)`.

Executable `0046d310` has exactly four switch cases: game tokens `0x70..0x73`
for ambient, diffuse, specular and emission. It copies one vector and clears its
corresponding care-mask bit. Other tokens return without changing state.
The four-component overload at `0046d2d0` dispatches to that vector setter.

Upstream 1.3.2 additionally handles `GL_AMBIENT_AND_DIFFUSE`, copying two vectors
and clearing a care bit. That extra case has been removed from the vendored
header, so combined material assignment is a no-op just as in the executable.
No change is made to `setColourMaterial`: its combined-mode enum remains valid.
Evidence is the canonical decompilation and the pinned records in
`manifests/plib_correspondence.json`. This change was determined by body review;
it does not require a new general-purpose library test.

## MG-PLIB-021 — preserve guarded network-message reads

Comparison base: **PLIB 1.3.2**, `src/net/netMessage.h`, `netMessage::geta`.

Executable `00415d90` copies and advances the cursor only when `pos >= 0`,
`pos < length`, and `pos+n <= length`. Otherwise both destination and cursor
remain untouched. The sampled upstream header comments out that `if` and uses
an assertion, leaving an unconditional copy in release builds. The vendored
header now restores the runtime guard. No extra error or exception path is
introduced. The unchanged seek clamp and network-order `puta` behavior are
recorded alongside this difference in `manifests/plib_correspondence.json`.

## MG-PLIB-022 — preserve selector virtual delegation

Against PLIB 1.3.2 `ssgStateSelector.cxx`, executable `00499db0..0049a029`
unconditionally calls the selected state's corresponding virtual method. The
upstream self checks (previously retained as typed base fallbacks) change this
behavior. They have been removed. Invalid or empty selection returns the selector
itself, so recursive behavior is intentionally retained, including for opacity,
care flags, materials and diagnostics. Corrected source roles in the recovery
baseline supersede shifted historical labels at `00499ed0..00499ff0`.

## MG-PLIB-023 — unconditional SSG diagnostics

Against PLIB 1.3.2, Entity `0047fc20`, Branch `0045b3e0`, Leaf `0047c460`,
State `0049afb0` and typed/simple-array printers `0047e860..0047eaff` have no
`how_much` suppression. Their extra upstream verbosity branches are removed;
the target API keeps its optional argument for compatibility. This extends
MG-PLIB-018's already restored Base output. No rendered behavior is involved.

## MG-PLIB-024 — executable SSG type flags and factory scope

PLIB 1.3.2 reassigned runtime type flags relative to this executable. The vendored
header now uses the constructor-evidenced bits: entity2, leaf4, branch0x10,
base-transform0x20, transform0x40, selector0x100, root0x400, cutout0x800,
TexTrans0x2000, VtxTable0x4000, VtxArray0x8000, state4, simple-state8,
state-selector0x10 and texture0x20000. Observed simple/typed arrays retain Base's
type1. The separate, unobserved VTable class retains its upstream bit and is
compiled out; VtxTable does not derive from it. Version-0 wire
mapping remains explicit but observed runtime entity/state IDs now agree too.

The unsupported 1.3.2 extensible factory and its references to unobserved classes
are compiled out in `ssg.cxx`. The evidenced closed version-0 factory remains in
`ssgLoadSSG.cxx`. This removes an unnecessary source of retained modern classes.

## MG-PLIB-025 — branch-only TexTrans class

Executable constructor `0048e670` and destructor `0048e6b0` use Branch directly;
the class has no transform matrix or texture-matrix cull method. Vendored
TexTrans now derives from Branch, and its unsupported matrix API is removed.
This completes the class adaptation behind MG-PLIB-019's branch-only payload.

There is an original defect: clone `0048e600` allocates 100 bytes, while copy
`0048e6f0` reads/writes the additional word at offset100. Its purpose is unknown.
The native class preserves that copy operation using `legacy_copy_word` with
defined storage initialized to zero. This is an explicit portability exception;
no claim is made that an out-of-bounds original value can be reproduced.

## MG-PLIB-026 — original deferred rendering queue

Against PLIB 1.3.2 `ssgDList.cxx`, the five executable functions
`0048ab20`, `0048ad10`, `0048ade0`, `0048ae30`, `0048aef0` use a 512-command
queue with opcodes leaf0/load1/push2/pop3/consumed4. The modern 4096-command
FIFO and texture-matrix commands do not match. The adapted queue preserves
load/push/pop coalescing, captures model-view matrices, sorts leaves stably by
descending depth, restores selector step snapshots and changes depth-write mode
from each leaf's render-order field. Modes >=2 get sort depth1000000; mode1
writes depth. The final depth mask is restored when necessary.

The x87 depth instruction order is confirmed by disassembly: y*m6, z*m10,
x*m2, radius, translation, negate, then float store. The target uses retained
double intermediates under the documented 53-bit precision profile. No pixel
rendering equivalence is claimed by this body comparison.

## MG-PLIB-027 — frustum FOV tangent and warning behavior

PLIB 1.3.2's `sgFrustum::update`, executable `004573b0`, rounded tangent to
float before multiplying the near distance. The executable calls float-input
adapter `004d9d30` (`FLD`, `FPTAN`, discard extra1, return ST0), retains the
result through multiplication and only then stores the extent. The vendored
math adapter and update now follow that order; the bottom extent negates the
retained top value before its store. Both invalid-input warnings go to target
stderr with the original text/newline, rather than the UL callback service.
The existing numerical differential includes additional FOV cases specifically
because this changed path involves FPU rounding.

## MG-PLIB-028 — NET virtual interface and callback details

Against PLIB 1.3.2, `netChannel::poll` and `loop` are receiver virtual methods
in the executable (`004676c0`, `004678c0`), and `loop` dispatches virtual poll.
The header now declares them virtual; the existing loopback fixture calls poll
through its client object. The original error callback has no argument; base
calls and the room-derived declarations now use that contract. Base diagnostic
prefixes use the original `Net:` text. Room entry resets local login state before
releasing discovery transport, matching `00468770` operation order.

## MG-PLIB-029 — font drawing precision and diagnostics

Against PLIB 1.3.2, `0045bb50` adds glyph x-position to the cursor before italic
offset, and retains advance through the cursor addition. The vendored routine
follows that order and its private return uses retained ST0 precision. Glyph
advance itself remains the MG-PLIB-014 field. Unrecognized font-extension output
at `0045bae0` goes directly to stderr with its original newline. The TXF loader's
gamma and upload paths were subsequently reviewed and adapted under MG-PLIB-056;
the remaining limits are recorded there and in the correspondence ledger.

## MG-PLIB-030 — PUI priority filtering and group hit behavior

Executable `00456600` accepts minimum/maximum draw priority and restores the
default range -99999..99999 after drawing. `0046a9e0` applies that inclusive
range to each child's priority; `00461040` initializes the widget priority0.
The native class now has a named priority property, and the unsupported
single-window display overload is replaced by the evidenced two-bound form.

Group hit testing `0046a920` walks children back to front, returning on a hit
after clearing highlights on the remaining siblings behind it. The upstream
floating/right-drag path is absent and is removed. Group stack/close diagnostics
use direct stderr as in the executable. The original group-stack capacity
predicate is preserved; no unrelated stack-bound fix is claimed.

PUI graphics setup `00456970` disables lighting, fog, texture, depth and culling,
leaving matrix/viewport setup to the surrounding game. Restore is a no-op.
The extra upstream attribute saves, projection setup and restores are removed.
Window identity/size still use the documented native Win32 provider.

## MG-PLIB-031 — PUI render callback and text state contract

Executable widget render callbacks take `(self, render_data)`; upstream 1.3.2
adds draw x/y arguments. The callback declaration, invocation helper and source
call sites now use the two-argument contract (Button `004651d0`, Text `0046a6c0`,
ListBox `0049ac00`). The corresponding unselected upstream call sites are kept
source-compatible without enabling their build units.

Font draw `004655b0` enables blending with SRC_ALPHA/ONE_MINUS_SRC_ALPHA, calls
FNT begin/puts/end, disables blending, resets blend ONE/ZERO, and disables texture.
The extra upstream attribute save and alpha-test setup are removed. No pixel
comparison is implied; these are instruction/call-order correspondence changes.

## MG-PLIB-032 — cached geometry counts and draw side effects

Against PLIB 1.3.2, Leaf constructor `0047c300` initializes four cached array
counts. VtxTable construction, copy, setters and version-0 loading publish them;
later array edits alone do not update them. Geometry loops and clamped accessors
now use the caches. Draw `0047f4b0` also increments the triangle counter.
Highlights restore polygon fill directly without changing line width. VtxTable
diagnostics retain the caller's indent without a verbosity gate. Individual body
reviews are in plib_correspondence.json. Unobserved AxisTransform, Invisible,
RangeSelector and VTable units are excluded from the explicit build list; their
distribution files remain.

## MG-PLIB-033 — cutout billboard scale and retained square roots

Original `0048e7e0`, compared with PLIB 1.3.2 ssgCutout::cull, preserves incoming
axis lengths in point-rotate mode, placing them on negative X, negative Z and
positive Y axes. Fourth components and translation survive. Upstream's fixed
axis-swap matrix loses those lengths. The cylindrical path retains square root
and reciprocal precision until component stores. The x86 helper follows the
float-input, ST(0)-return FSQRT adapter `004d9d50`. This numerical adaptation
requires a focused original comparison before closing the cull review.

## MG-PLIB-034 — global texture registry and lifetime

Original constructors `0048ddd0`/`0048de40` insert every texture into one global
linked list. Destructor `0048df20` releases handle, filename, then list membership
before Base destruction. Lookup `0048dd90` uses ulStrEqual filename; helper
`0045ad20` reuses that object regardless of wrap/mipmap arguments without adding
a cache reference. Vendored PLIB 1.3.2 now follows this instead of the options
texture array. Every nonzero handle is deleted, including one from setHandle.
Print `0048e010` emits the original literal "whatever". Filename storage uses
matched array deallocation under MG-PLIB-004. Image decoding, embedded PIX and
image dispatch were subsequently addressed in MG-PLIB-037/040/059; their
individual limitations remain recorded separately.

## MG-PLIB-035 — legacy parser return values and conversion rules

Against PLIB 1.3.2, `004b5720` returns null at line end rather than an EOL string.
`004b5770` returns the unquoted string pointer, retaining its original numtok-only
guard. `004b57f0` returns atof as double; `004b5810` returns atoi. Neither checks
trailing characters. The vendored parser now exposes these original contracts;
small overloads support the retained upstream ASE call sites. The modern parser
open/error services and ASE loader adaptations remain separately under review.

## MG-PLIB-066 — ASE material parser error boundary

Static review against PLIB 1.3.2 and executable entries `0048c950`, `0048c9d0`,
and `0048ccf0` found corresponding material-list traversal, material defaults,
submaterial numbering, colour and texture fields, and filename stripping for
well-formed quoted strings. The retained compatibility overloads return failure
for a missing quoted string, whereas the executable's callers can proceed with a
null pointer; malformed input is not claimed equivalent. These body reviews do
not establish equivalence of the containing ASE loader, converter, or renderer.

## MG-PLIB-067 — file picker selection and x87 scrollbar arithmetic

The executable's file-selector bodies at `0046ac90`, `0046ad90`, `0046b1a0`,
`0046b2c0`, `0046b380`, and `0046b9b0` through `0046bd30` correspond to the
PLIB 1.3.2 `puFilePicker` class and its helpers. The formerly omitted source
is now selected in the build. The two scrollbar callbacks retain intermediate
precision through integer conversion, matching the original x87 instruction
order. The arrow callback retains the final intermediate through integer
conversion and double-precision formatting while storing a float and updating
bound resources, as shown by `0046ba87`–`0046bab7`. Filename arrays use `delete[]`
to match their allocation on the target runtime. Constructor and directory-service
details and runtime comparison remain open. Directory enumeration retains the
two-pass file count/copy and sorting behavior. The target `ulOpenDir` returns
null when the Win32 search fails; the executable's outer iterator could instead
contain a null inner iterator and be dereferenced. Failed searches therefore
have a defined empty result here rather than reproducing that crash.

## MG-PLIB-068 — ASE temporary record ownership

Static review against PLIB 1.3.2 and the executable's `00463260`, `00463280`
and `0048dbb0` confirms zero-initialized mesh records and the release order for
mesh arrays and material records. Filename and material-name arrays now use
`delete[]`, matching their allocation on the target runtime; the executable
calls its allocator's deletion service directly. The executable's node record
also stores a kind field absent from upstream `aseObject`; MG-PLIB-069 adds it.
Later entries describe the node-processing implementation and its remaining limits.

## MG-PLIB-069 — ASE node kinds and camera point geometry

Against PLIB 1.3.2, executable `0048c6e0` recognizes geometry, helper and
camera objects and `004632b0` initializes their kind. `0048ce50` reads named
`NODE_TM` blocks, node/target positions and inheritance flags. Those fields and
branches are now represented in the vendored parser. For a face-free camera,
`00463ba0` creates one position point or a position and normalized target point;
other face-free objects use mesh vertices, with no material state and culling
disabled. Empty objects still receive a branch. Camera normalization follows
the statically observed arithmetic order; runtime parity has not been compared.
Node creation, animated/multiple meshes, inheritance behavior, converter output,
error handling and the full ASE loader remain under review.

## MG-PLIB-070 — ASE mesh indices and named animation samples

Against PLIB 1.3.2, `0048d230` parses face indices with the parser's atoi-style
numeric-prefix conversion; the upstream stricter `strtol` check is removed.
`0048d880` accepts animation samples only after a matching node name, and for a
parented object subtracts position components whose inheritance flags are set.
Those branches are now present; the unparented Z subtraction retains the
parsed double through subtraction as in the executable. The sample allocator `0048dae0` otherwise
matches upstream initialization, frame calculation, diagnostic and count update
for valid times. Malformed input and the complete animated scene builder remain
open; no current original runtime comparison is claimed.

## MG-PLIB-071 — ASX multi-frame selectors

`0048af50` reads `meshCount` records into consecutive node slots and
`00463330` builds an `ssgSelector` when there is more than one mesh: a single
material places a leaf directly in the selector; multiple submaterials produce
one branch per frame. Step zero is selected. The retained ASX reader now builds
that topology instead of rejecting multiple frames. Unknown object kinds follow
the executable's non-camera helper path. The aim-point flattening operation
`00463ea0` now recursively visits branches and selector children. A reconstructed
two-frame fixture checks selector topology and initial selection; original
runtime parity and sparse/stale serialized mesh pointer cases remain open.

## MG-PLIB-072 — ASX cursor and chunk tags

The reader's `take` and `marker` implement executable helpers `0048b2c0` and
`0048b2f0`: an empty span returns null without advancing, nonempty spans advance
the cursor, and a mismatched four-byte tag fails. Unlike the executable's
unchecked cursor and fatal exit, the native reader checks bounds and throws an
exception on truncation or a bad tag. The caller's complete error behavior and
full ASX scene reader are not claimed equivalent.

The ASE-to-ASX converter was subsequently implemented in MG-PLIB-076. Existing
static inventory from
`recomp/audit_asx_converter_globals.py` shows the output handle `0054d854` and
embed flag `0054d860` are zero-filled initially; their literal code references
are reads or tests, with no literal store or address-taking instruction found.
This suggests dormant converter code in this executable, but does not prove no
indirect write can activate it. MG-PLIB-076 supplies the writer paths; their
static reviews record remaining edge cases and do not claim runtime parity.

## MG-PLIB-076 — ASE-to-ASX converter

The executable contains a separate, apparently dormant `ssgConvertASX.cxx`.
Its material and object records, aligned strings, exact writes, terminator,
two-pass PIX validation and embedded PIX records are reconstructed in the
selected SSG source unit. The ASE parser supplies temporary material/node
views before constructing each scene node and finalizes before material cleanup.
The original mechanism that sets the output globals is not evident: an internal
configuration entry explicitly supplies a caller-owned stream for testing and
future integration. The caller supplies and updates the seven-word ASX header;
the observed converter bodies only write the records and trailer. Serialized
pointer placeholders reflect the current 32-bit process and are not stable byte
identities. Failed I/O diagnostics, stale sparse mesh pointers, format edge
cases and original runtime equivalence remain open.
Exact-write failures and mesh-count mismatches use the target runtime's
`exit(1)`, matching the executable's fatal service even if a PLIB error callback
is installed. Finalization runs even when the output stream is null, preserving
the separate embedded-texture flag behavior.

## MG-PLIB-077 — shared camera endpoint arithmetic

Executable `00463ba0` first computes direction length in z/y/x order and
rounds the squared length to float, then recomputes the dot product and takes
an x87 square root. Its normalized X and Y are rounded before adding position;
Z is retained through its addition. ASE and ASX now share one endpoint helper
with those store boundaries. Zero-length direction produces only the position
point. Nonfinite values and original runtime comparison remain unverified.

## MG-PLIB-078 — ASX leaf callback and absent attributes

Executable builders `00463ba0` and `00463f90` pass each constructed leaf and
its node name through the active loader's `createLeaf` callback. The ASX path
now does so, including the default state-sharing behavior. It supplies null for
absent normal, texture-coordinate and colour constructor arguments, as in the
original; the PLIB vertex-table constructor then creates empty arrays for those
arguments. Callback replacement ownership and failure paths
remain dependent on the loader's callback contract; native tests cover the
default callback and an observing override.

## MG-PLIB-082 — ASX opening and record tolerance

The recovered `0048af50` joins the global model directory and returns an empty
root, without `endLoad`, if opening fails. It consumes recorded string lengths
without checking padding contents or requiring alignment, accepts float bit
patterns without a finiteness check, and does not reject bytes after the final
tag within its buffer. The native reader now follows those cases. It still
checks bounds and requires a string terminator, unlike the unchecked original;
truncated, invalid-index and other malformed records remain adaptations.

## MG-PLIB-083 — indexed mesh comparison

The recovered indexed branch of `00463f90` uses ordered equality for UV and
colour components. PLIB 1.3.2's zero-tolerance comparison treats unordered NaNs
as equal; the shared ASE/ASX indexed conversion now treats them as different,
as the recovered comparisons do. A native fixture covers a filtered
submaterial and both duplicate and NaN-separated vertices. The native 16-bit
index-overflow guard remains an explicit divergence from unchecked original
casts; original runtime parity has not been tested.

## MG-PLIB-084 — mesh state construction order

The recovered `00463f90` obtains material state before expanding an unindexed
triangle mesh, but after remapping an indexed mesh. The shared ASE and ASX paths
now preserve that distinction, including loader material callbacks and texture
side effects. Individual allocation failure behavior remains target-dependent.

## MG-PLIB-079 — loader completion callback

Executable ASE `0048c620` and successful ASX `0048af50` both invoke the active
loader's `endLoad` callback after parser/material or read-buffer cleanup. The
corresponding vendor loaders now do so; the general `ssgLoad` dispatcher still
does not add a second callback. The default callback `0045ad10` clears only the
shared state pool, not the upstream compatibility texture pool. Native tests
cover a custom ASX callback; failure
and exception behavior remains distinct and is not claimed equivalent.

## MG-PLIB-087 — scene-loader primitive tokens

The existing recovered graphics-token table `00523960` maps game primitive 4
to OpenGL `GL_POINTS` (0) and primitive 8 to `GL_TRIANGLES` (4). The camera
and helper builder `00463ba0` passes 4; mesh builder `00463f90` passes 8.
The ASX geometry adapter's two corresponding native modes therefore match
those recovered mappings. The review exporter checks both table entries. This
does not establish the rest of the renderer's behavior.

## MG-PLIB-086 — rebuild aim-node display lists

The decompilation of `00463ea0` deletes and recompiles each leaf's display
list after translating and flattening its vertices and invalidating its bounding
sphere. Both the ASE and ASX aim-node paths now do the same. This is a static
comparison; renderer behavior has not been compared with the original at runtime.

## MG-PLIB-085 — default loader leaf finalization

The existing decompilation of `0045acb0` shows that the loader's default
leaf handler shares states only when their type is exactly `0xd`, then
compiles the leaf's display list after the final state is selected. PLIB
1.3.2's handler accepts derived simple states and does not compile the list.
The vendored handler now uses the exact type and calls `makeDList` when
display lists are enabled. This is a static comparison; OpenGL behavior has
not been compared against the original at runtime.

## MG-PLIB-081 — animated-transform factory

The recovered default loader factory `0045ada0` returns the supplied transform
if its matrix array is null; otherwise it deletes that transform and constructs
the game animated-transform class with the array. The vendor default now does
the same, retaining ownership of the matrix array. Both ASE and ASX paths call
the active virtual factory so custom loader options can replace its behavior.
The target animated class retains the array and reads its current matrix values,
while retaining the initial frame count as in the recovered constructor.

## MG-PLIB-080 — ASX embedded stream lifetime

The decompilation of `0048af50` closes the embedded PIX stream and resets its
global context before checking the final scene tag; it frees material/read
storage before calling `endLoad`. The native reader now closes its scoped
embedded context at that point and releases temporary material/read storage
before the callback. A fixture checks that the embedded loader is no longer
installed when a custom `endLoad` callback runs.

## MG-PLIB-073 — ASE/ASX material grouping and colours

Against PLIB 1.3.2, executable `00463f90` uses white RGB for a non-vertex-colour
mesh and uses alpha 1 when `0050781c` disables material transparency. Both the
ASE and ASX geometry paths now observe that flag; indexed colours do likewise.
The single-material path selects sentinel submaterial 9999, and IFL parent
materials count as one geometry group while their state selector still counts
their submaterials. The full game-specific mesh conversion remains under review.

## MG-PLIB-074 — shared ASE and ASX mesh construction

The executable uses the same `00463f90` mesh builder for ASCII and binary
nodes. The ASCII loader now passes parsed mesh and material data to the same
game-derived indexed/triangle geometry routines used by the ASX loader, then
applies the PLIB loader's state and leaf callbacks. The original 1.3.2 body is
retained but compiled out. Selection, UV/colour remapping and alpha settings
therefore share one implementation. The full original mesh function, graphics
callbacks and edge cases remain under review; no current original differential
comparison is claimed.

## MG-PLIB-075 — ASE animation frames and aim nodes

Executable `00463330` uses the same scene-building behavior for ASE and ASX.
The ASE path now uses the shared reconstructed transform-frame arithmetic,
including scaling of rotation columns, while retaining the loader's transform
factory callback. Names ending in `aim` get recursive geometry flattening,
a cutout and a pivot translation before any animation wrapper. The PLIB 1.3.2
animation body remains present but compiled out. Native fixtures cover scaled
frames and aim topology; full scene-builder and callback parity remain open.

## MG-PLIB-036 — original material application order and cache behavior

PLIB 1.3.2 disables capabilities before material updates and enables them at the
end. Original apply `0046c310` and force `0046c690` update materials first, then
enable before disable, followed by texture, shade and alpha operations. Apply
forces cared-for material uploads when colour-material mode changes. It does
not use upstream's -1 cache invalidation. Force also leaves uncared-for cached
colours untouched. RGB caches retain their fourth component; diffuse uses RGBA.
The vendored bodies preserve this order. A focused capture executes original
bodies and adapters, comparing ordered native GL calls and all colour-cache
components/capability bits for128 fixtures per body:1536 checks passed.

The complete focused oracle also passed256 cutout checks from MG-PLIB-033,
for6244 checks over229 original entry points. These are bounded comparisons,
not full-game rendering equivalence. The former selector clone fixture was
invalid: State::copy_from invokes an empty destination selector virtually before
its steps are installed, causing the original recursive fallback. The test now
checks supported selection/refcount behavior without assuming that clone works.

## MG-PLIB-037 — executable image loaders and mipmap ownership

The fixed registry at `00520c40` contains PIX (`004b5f90`, historically mislabeled
PNG), paletted TGA (`004b5fe0`) and PNG (`004b5f40`). BMP/SGI units and registration
are now explicitly compiled out. PIX, embedded PIX and the fixed dispatcher were
subsequently implemented under MG-PLIB-040/059; their edge-case limitations
are recorded in those entries and in the correspondence ledger.

Against PLIB 1.3.2, TGA decoding is replaced by the executable's uncompressed
8-bit indexed format:256 palette entries, RGB/RGBA palette, zero origins, no
right-to-left ordering, explicit row orientation, BGR conversion and palette-wide
alpha classification. Opaque RGBA palettes expand to RGB. Success closes FILE
instead of reproducing the original leak. Vector-backed temporary storage has
defined values on short reads, where the original uses indeterminate allocation
contents; malformed/truncated input equivalence is not claimed.

Original upload `004b63f0` delegates to glpng Build2DMipmaps with filtering enabled.
The vendored adapter now does likewise, without upstream PLIB proxy uploads,
alignment changes, texel accounting or image ownership transfer. Invalid sizes
warn using the last TGA filename and produce the red/white2x2 fallback. The native
fallback frees its allocation instead of retaining the original leak. Texture
construct/copy/load consume the shared alpha flag; PNG updates it only on success.

Twelve original/native TGA upload streams (including every generated mip level)
and nine palette-alpha results passed. Cases include both orientations, RGB and
opaque/nonopaque RGBA palettes, and unsupported true-colour header fallback. The
original renderer is explicitly set to its32-bit upload profile for this fixture.
This is not verification of all glpng PNG-decoding behavior or16-bit uploads.

## MG-GLPNG-003 — unified image input streams

Comparison base: vendored glpng **1.45**, with existing MG-GLPNG-001/002
changes. The exact original release identity remains unproved.

Saved PIX `00462ab0` and glpng `00461ac0` read the supplied handle directly
through shared read callback `00524668`; PIX uses shared seek `0052466c`.
Saved `png_default_read_data` (`004b6640`) reads `png->io_ptr` through that
same shared read function and calls `png_error("Read Error")` on short reads.
The bodies contain no temporary-file spooling. The earlier reconstructed MPX
adapter did spool to FILE; that was an implementation divergence and is removed.

`MegustaReader` provides borrowed context/read/seek functions. Stdio wraps FILE;
mounted native files and archive memory wrap ResourceFile. PIX stdio and mounted
entrypoints share one payload decoder. glpng's existing pngLoadF/pngLoadRawF/
pngBindF entrypoints adapt FILE and call the same reader-based decoder/binder
used by mounted resources. libpng uses its public `png_set_read_fn` callback.
There is no temporary file, full-resource copy, global reader switch, fake FILE,
or separate resource decoder. The reader and resource outlive synchronous decode.

Compatibility differences are explicit: the original globally redirected CRT
dispatch is represented by per-call typed callbacks; public reader entrypoints
are new APIs, not recovered ABIs. libpng callback errors preserve the saved
short-read failure path. A truncated eight-byte signature now fails immediately
instead of checking partially initialized signature storage. Existing upstream
setjmp/error cleanup remains in use; decoder algorithms and PIX numerical/GL
behavior are unchanged by this input refactor. Original global PNG scratch is
still normalized to local per-call info where previously documented.

The model-authored incremental patch is
`patches/glpng/0003-reader-stream.patch`; `tools/codegen/prepare_glpng.py` applies it
to its MG-001/002 baseline, verifies pristine pins and reproduces vendor files.
`0001-mipmap-bridge.patch` remains the generated aggregate C-source diff against
pristine 1.45; it is not a baseline to which 0003 should then be reapplied.
The incremental patch also records the public header and new reader header.
Native implementation files are `src/engine/reader_stream.h`,
`pix_texture.cpp` and `multi_texture.cpp`.

Checks exercise an in-memory one-pixel PNG through the actual vendored raw
decoder, item-count read/seek semantics and both signature/chunk truncation.
No filesystem or GL context is needed for those checks. Actual texture upload
remains outside this test; no original executable or new Ghidra analysis was used.

## MG-GLPNG-001 — private bridge to existing mipmap algorithm

`vendor/glpng` is the retained glpng1.45 distribution plus one bridge exposing its
existing static Build2DMipmaps function to SSG. The algorithm is unchanged.
`tools/codegen/prepare_glpng.py` reproduces it from the pinned input already inside
`upstream/`, with a final diff at `patches/glpng/0001-mipmap-bridge.patch`.
glpng1.45 remains a comparison base, not a proven exact release identity.

## MG-PLIB-038 — compiled display-list leaf derivative

`ssgCompiledLeaf` is reimplemented as a C++ class from executable assertions,
constructor `0047e1f0`, draw `0047e310` and vtable `004ebf00`. It derives from Leaf,
retains cull/render-order/state, moves an existing nonzero display list, copies
the sphere and keeps compiled draw counts separately from zero inherited counts.
Its public triangle count returns0, matching the shared zero-return slot; draw
uses the separate compiled count. Getters, triangle extraction, transform,
print/load/save terminate in the original and in the reimplementation. Highlight
and geometry slots do nothing. Additional native PLIB collision/pick/line slots
have no executable counterparts and remain compatibility-only empty operations.
This is a game derivative, not unchanged upstream code. Original constructor/draw
comparisons passed ownership, sphere, list dispatch and all three statistic deltas.

## MG-PLIB-039 — version-0 key and header edge behavior

Original key insertion `0048b4d0` warns for key0 but still stores it; the native
registry now does likewise and uses initial capacity10 for that case. Negative
keys and the original exact-capacity growth overrun retain safe native bounds
handling instead of memory corruption. The version-0 reader now distinguishes
wrong magic, byte-swapped magic and unsupported versions with original messages.
The writer emits unknown runtime type tags and invokes virtual save rather than
rejecting them preemptively. This preserves compiled-leaf's terminating save
behavior. Broader version-0 top-level error-path correspondence remains open.


## MG-PLIB-040 — fixed image registry and PIX service

Fixed registry00520c40 is PIX/TGA/PNG, available before ssgInit. Dispatch now
preserves subdirectories, resets alpha and returns after a recognized loader.
Missing extensions warn without fallback; unsupported formats upload fallback.
The expandable image registry is compiled out. Embedded ASX integration remains
open: a resolver boundary exists, and the current reader explicitly rejects it.
PIX remains separate in src/engine/pix_texture.*, supported by src/pix/Pix.cxx
and PixUnpack.cxx strings. Named52-byte headers/16-byte levels, max-size skipping,
codecs2/4, zlib1.1.3 dispatch5..13, mipmap modes and alignment restoration follow
original bodies. Twelve fixtures passed36 original upload/metadata comparisons.
Zlib dispatch is source-reviewed, not exercised there. Bounds guards/defined
short reads replace malformed-input corruption; that equivalence is not claimed.

## MG-PLIB-041 — game ASE material policy and UV precision

ssgASESupt.cxx restores00464dd0's filename/material-index factory, transparency
flag/threshold0.01 and texture alpha, disabled alpha-test/lighting, blend and
smooth shade. Both ASE and ASX now use it; ASX installs material states and
preserves null/empty filenames. IFL with fewer than two frames warns/falls back
to simple state; otherwise a selected state selector is built. UV helper00464f20
retains1-v across its float store, then rounds each multiply/add at observed
stores. Both geometry paths share it. Original tests passed256 UV and28 material
cases; image services are independently scoped. Legacy parser now uses void
openFile, rt mode,256-byte diagnostics, no EOF sentinel fields, and atoi-compatible
unsigned glue; unobserved line-independent parser APIs are compiled out.

## MG-PLIB-042 — context, lights and renderer profile

Context00459700 lacks clipping-plane storage/setup and the extra blend call;
those extensions are compiled out. Context FOV uses3:2 fallback;00459e60 is a raw
matrix store. Cull/draw accepts Entity.0045a070 entry/exit GL order, light passes
and frame increment are restored. Unobserved spotlight extensions are compiled
out. Light defaults match the original19 fields of PLIB1.3.2. Previously omitted
CRT entries459f00/459f10 (pointer004f20c4) initialize eight76-byte records; target
C++ global construction replaces that compiler assembly, with addresses retained.
SG double evaluation now propagates to inline/header consumers; explicit float
casts preserve storage boundaries. Assertions remain active in every build
configuration. Flags/build success do not establish all-input numerical parity.

## MG-PLIB-044 — SSG source ownership correction

ASX loading, geometry construction, ASE indexed/triangle conversion and animated
transforms now live under vendor/plib/src/ssg and compile directly into plib_ssg.
Old src/engine copies and megusta_engine are removed. Embedded source paths at
00463330 (ssgASESupt.cxx) and0048af50 (ssgLoadASX.cxx), and PLIB1.3.1/1.3.2 geometry
fragments establish SSG ownership. Animated-transform ownership is inferred from
SSG inheritance/vtables; its private original spelling is unknown. Native file
splits do not claim original translation-unit identity. PIX remains separate.
Every move is in manifests/ssg_source_relocation.json; addresses/statuses remain
in the global index. Relocation alone does not verify code.

## MG-PLIB-045 — animated transform history and all-frame bounds

Restore constructor0047db30's bit0x1000000. Setter0047dc30 clamps signed frame,
updates history/current/first-time state and does not dirty the all-frame sphere.
0047de10 unions children, then their centers through every frame without scaling
radii. Target vector ownership replaces private matrix-array storage; this is a
container/layout adaptation. Empty sequences can be constructed; selecting one
terminates explicitly instead of reproducing null access. Original transform
vtables inherit Branch print; the extra upstream matrix print override is removed.

## MG-PLIB-046 — PUI cursor extension and shutdown

PLIB1.3.2 only supplies a triangle cursor. Executable00456380/00456410/
00456420/00456670 adds texture handle, hotspot offsets, dimensions and idle-fade
state. Restore the textured quad and exact ordered GL state calls, six-second
idle delay, three-second fade, five-pixel motion threshold and retained-epoch
flag. Use the target timeGetTime service with signed32 tick interpretation and
float constant0.001f. Keep the retained fade calculation until its output float
store. Twenty-four bounded original comparisons pass complete GL streams, including
fade float bits and a signed-clock boundary. This is not a full rendering proof.
Public API names are recovered role names.
Add shutdown004565e0 (cleanup then delete ultimate live interface). Mouse button
shifts explicitly mask by31, preserving x86 behavior without C++ undefined shifts.

## MG-PLIB-047 — fixed model loader registry

Read binary table00506c40: ordered ASE(load0048c620/save0048c5c0),
SSG(0048b5b0/0048b940), ASX(0048af50/0048b320), then null sentinel.
Replace startup-only dynamic registration with those three initial records;
repeated ssgInit no longer appends duplicates. Load uses case-insensitive
extension-prefix comparison of the registry extension length, as0045aed0 does,
and does not perform the upstream extra endLoad call. ASX load reaches the
reconstructed reader; embedded and multiframe paths were subsequently added in
MG-PLIB-059 and MG-PLIB-071. ASX save terminates unconditionally, matching0048b320. Native optional
loader-options arguments remain compatibility glue. The old ssgAddModelFormat
API is retained for source compatibility; no executable body is claimed for it.

## MG-PLIB-048 — room discovery and current-room accounting

Restore00468560: return null unless connected/past login with an owned room;
recount games/players over IDs1..255, sum open-game capacities then subtract
players assigned to non-running games. Restore00468640: when the discovery
socket exists, send the five-byte type207/to0/from0 query to room host/port.
Restore original discovery failure and room-info diagnostics. These additions
are binary-derived extensions to PLIB1.3.2 netRoomClient; role names are inferred.
No network interoperability or remote server claim follows from local checks.

## MG-PLIB-049 — scene animation controller

Reimplement the binding-named ssgAnim as an ssgBase subclass in
ssgGameAnimation.h/cxx, alongside the SSG extension rather than game root code.
Addresses0046d480..0046d808 show frame counting, user-data attachment/ref,
pre-traversal callback and recursive selector/animated-transform/material-state
frame application. setFrame stores the unsigned frame and defers application
while attached. Child traversal precedes branch-specific frame application.
The entity owns its animator through userData; the animator entity pointer is
non-owning. Native compiler layout/exception machinery replaces VC6 layout.
The otherwise out-of-bounds empty node-selector write terminates explicitly.
The original callback0046d7f0 is an interior entry, retained as evidence rather
than inventing another function inventory row.

## MG-PLIB-050 — ASE exporter behavior

Compared against PLIB1.3.2 ssgSaveASE.cxx and original0048ba50..0048c611.
The game allocates a non-owning1000-pointer state table, collects VtxTable
SimpleState pointers by identity over every branch child, exports all selector
children, and does not reject non-triangle primitive modes. Textured UV output
uses the stored components directly with Z0 and no texcoord-count gate. Native
vector storage preserves non-owning order;1001st unique state aborts instead of
reproducing original buffer overrun. Two complete original output comparisons
pass: empty scene and untextured geometry repeated under a selector. Material
and textured branches are statically reviewed, not runtime-proven by those cases.

## MG-PLIB-051 — game statistics output

Original0046d810 reports exactly TRI=%3d, TEXBIND=%3d with no newline, then clears
vertex, triangle, leaf, cull and texture-bind counters. Replace upstream long
statistics report/reset. Unobserved upstream collision counters remain native
compatibility state and are not reset by this entry.

## MG-PLIB-052 — global loader paths and material sharing

Original0053dde0/0053dde4 hold model/texture paths globally;0053ddf8 is the
one capacity3 state-sharing array. PLIB1.3.2 places them in each options object.
Restore shared static ownership. Provider constructors/destructors no longer
reset/free global paths or construct private material pools. Native C++ static
construction/destruction replaces original startup/atexit wrappers. Match array
allocation/deallocation and null path pointers after explicit release (a target
cleanup adaptation). Historical helper labels0045ae60/0045aeb0 have model and
texture reversed; the accessed globals and setter bodies establish ownership.
The native optional texture pool remains compatibility storage; original cache
semantics use the already restored global live texture list.

## MG-PLIB-053 — complete version0 top-level disposition

Original0048b5b0 accepts exactly eight concrete entity tags at the root; generic
field-object null/reference/material grammar does not apply there. Restore that
switch and failed-open/child read/write diagnostics; writer emits root tag then
virtual save directly. Existing original round-trip fixtures cover supported
normal streams. Native closes original error-path file leaks, frees a failed
new object, initializes values on short reads, guards null save roots and
negative/equality key bounds, and reuses vector capacity between read sessions.
These are explicit robustness/ownership adaptations. Native path normalization
uses the shared target path service; original v0 reader/writer joined model
paths without backslash normalization. Valid Windows paths are accepted by
both; diagnostic spelling of such paths can differ. No malformed-input memory
corruption or failed-allocation crash is asserted equivalent.

## MG-PLIB-054 — list-box click semantics

Original0049ade0 ignores the event edge when the button is left: clear highlight,
calculate/clamp selected row, update value, invoke callback and only then publish
this as active widget. PLIB1.3.2 gates on active_mouse_edge and publishes before
the callback. Restore original ordering and remove the edge gate. The unused
updown/x arguments remain in the native method interface.

## MG-PLIB-055 — current context shutdown

Original0045a050 deletes the current SSG context when present. Add the native
ssgShutdown entry, delegating cleanup to the existing context destructor.

## MG-PLIB-056 — TXF upload and filtering

Original0048f2c0 omits the GL-context gate, disables half-texel offsets for
nearest/linear minification and uploads a single level with saved/restored unpack
alignment. Other filters use the shared glpng filtered mipmap implementation.
Restore these behaviors and retain the input space glyph. Native error paths
close the file and release arrays rather than reproducing original leaks.
Byte-format luminance uses the shared MG-GLPNG-002 gamma lookup while alpha
retains the raw value. Six TXF fixtures match original glyph fields and complete
upload streams for byte/bitmap formats and nearest/linear/mipmap filters.
Error diagnostics use the target UL warning service rather than the original
direct stderr formatting; malformed inputs and all gamma values are not proven.

## MG-GLPNG-002 — shared byte gamma lookup

Original00461990 initializes an identity table then applies a gamma curve when
gamma is positive and unequal to1;004623c0 calls it when viewing gamma changes.
The same lookup modifies the TXF byte image luminance, leaving alpha unchanged.
The glpng1.45 base is modified by the reproducible prepare_glpng.py generator.
Original lookup comparison passed all256 input values for gamma0,1,0.5,1.2,
1.45 and2.2. The original x87 pow implementation and target runtime pow are not
proven identical for all possible gamma values.

## MG-PLIB-057 — PUI input interactions

Compare PLIB1.3.2 puInput with00469db0/00469e50/0046a250/0046a3e0. Restore
selection-polygon vertex order, fixed seven-pixel cursor-bar bounds, original
mouse-edge activation and callback-before-active order. Remove upstream drag
selection, clipboard shortcuts and valid-character filter; backspace deletes
one character even when selection exists, while Delete removes the selection
without moving the cursor. Native fixes the original left-edge mouse underflow
and selection-copy read beyond NUL. Text binding and less common edge cases
remain pending original comparison.

The executable's GUI factory type 5 is a game-owned, compact input object.
At0040bc20 it allocates 0x1c0 bytes, calls `puObject` directly, and initializes
four 32-bit input-state fields at offsets432–447. Its vtable combines the
`puObject` deleting destructor with the PLIB input draw/hit/key routines. The
routines at00469e50,0046a250 and0046a3e0 access the base plus those four fields
and never read `puInput::valid_data`. This object distinction is documented
with the base-class variant in MG-PLIB-091 below.

## MG-PLIB-058 — PUI colour calculation

Original00460f10 compares each 1.3f product with1 before its float store.
Preserve the retained intermediate through the clamp instead of narrowing it at
the helper call. Twenty-four bitwise original comparisons pass across six
colour sets, including values near the clamp boundary.

## MG-PLIB-059 — ASX embedded PIX records

Original0048af50 retains the input file when its embedded count is nonzero,
with the first record at the scene-data end offset. Original0048b340 strips
the final filename extension and scans 28-byte feedface records, advancing by
28 plus the stored payload size; matching names dispatch the PIX payload reader
and update shared alpha. Install a scoped embedded loader during scene assembly
and restore the previous loader on exit. Native checks bounds, name termination
and offset overflow and throws on malformed records, replacing original unsafe
reads. A constructed scene with two embedded records passed comparison against
original0048b340 for matched/missing names and the full PIX upload stream;
callback restoration was also checked. Other object/frame cases remain open.

## MG-PLIB-060 — retained scene dot product

Original00463e80 multiplies and sums Z, then Y, then X, returning the x87
accumulator without a float store. Add sgScalarProductVec3Retained beside
PLIB1.3.2's float-returning helper and use it for ASX camera-direction
calculation. Four direct original comparisons pass under the oracle FPU control
word, including cancellation; this is not an exhaustive numerical proof.

## MG-PLIB-061 — NET error text and retrieval

Original00469bf0 consumes the room-client pending error, advances login/join
state to ready when an error is present, then returns0046e750's text. Add
getErrorString() alongside the upstream numeric getError() compatibility API.
Restore the original fixed protocol error strings and unknown-code format;
small system errors use the target CRT strerror service. Direct original
comparisons passed for no-error, code61, codes100–114 and an unknown code.
The target CRT's system-error wording may differ from the original CRT.

## MG-PLIB-062 — one-time NET initialization

Executable004680a0 forwards to004dc350, which calls WSAStartup(1.1) and
registers cleanup only once, setting its flag only after success. Add the same
once guard to PLIB1.3.2 netInit; target platform services supply Winsock.

## MG-PLIB-063 — bound PUI input insertion

Original0046a3e0 inserts printable characters into its internal text buffer,
then synchronizes from an external bound string when present. This discards the
insertion for bound strings while still advancing the cursor. Restore that
behavior. Twenty direct comparisons cover bound/unbound strings, selection,
backspace, delete, clear, printable insertion, navigation and ignored keys.
MG-PLIB-057 documents deliberate bounds-safety adaptations.

## MG-PLIB-064 — include the evidenced PUI frame unit

Original file-selector construction creates a type0x10 frame with vtable
004ebb64; draw slot0049a990 corresponds to PLIB1.3.2 puFrame::draw, previously
mislabeled as a button renderer. The upstream source matches and is now in the
explicit build scope. It uses the existing documented callback ABI adaptation.
PUI box styles were compared against original004657d0 for all -9..9 styles,
with and without the default outline; native glRecti was normalized to the
equivalent polygon command sequence for comparison.

## MG-PLIB-065 — PUI slider integration

The executable contains slider draw and pointer-event methods at00466a40 and
00466ca0. Add the PLIB1.3.2 slider unit to the explicit build, preserve the
retained draw arithmetic, remove parent reordering and publish the active widget
after invoking its callback. Static body reviews cover ordinary finite inputs;
the original runtime comparison remains pending.
The attempted original-code fixture timed out before a result, because the
suspended original process did not run VC6 floating-point formatter startup.
Do not execute the shipped original or retry its fixture without explicit
user authorization; future comparison uses existing recovery exports.

## MG-PLIB-088 — omit upstream SSG collision traversal interface

Compared with PLIB 1.3.2. The recovered constructor-linked vtables have no
named `hot`/`isect` slots for `ssgEntity` (`004ec000`), `ssgLeaf` (`004ebd60`),
`ssgTransform` (`004ebddc`), the game animated transform (`004ebe44`),
`ssgSelector` (`004ebeac`), `ssgCompiledLeaf` (`004ebf00`), or `ssgVtxTable`
(`004ebf7c`). `recomp/audit_ssg_collision_vtables.py` reproduces the slot
inventory in `manifests/evidence/ssg_collision_vtables.json` from the existing
decompilation and vtable export.

The Win32 PLIB target now defines `MEGUSTA_NO_SSG_COLLISION`, guarding the
corresponding upstream virtual declarations, implementations and traversal
helpers. The two no-op `ssgCompiledLeaf` overrides are also omitted under this
interface choice. `src/ssg/ssgIsect.cxx` is excluded from the build allowlist.
The current `megusta_scene_inspect` target builds with this source closure, and
the generated PLIB patch reproduces every byte in the vendored PLIB working
tree (`python3 tools/codegen/prepare_plib.py --verify-rebuild`).

Evidence is limited to recovered vtable slots and the current target's source
closure. It supports omitting the virtual traversal interface; it does not
prove that the original executable contains no standalone intersection helper.
This remains a high-confidence interface reconstruction, not a full binary
identity claim.

## MG-PLIB-090 — compact executable PUI group and input classes

The executable's group constructor (0040ded0) initializes only child count and
list pointer after the 432-byte puObject base; the factory allocates 440 bytes.
Remove upstream dragging/floating storage and initialization. The already
adapted group event methods have no dragging path. Floating accessors remain
stateless compatibility methods for excluded upstream menu sources.

The input factory allocates 448 bytes and initializes four integers following
puObject; 0046a3e0 does not inspect a valid-character whitelist. Model this as
puInput directly deriving from puObject with those four fields. Remove
valid_data and its whitelist API from this class (puLargeInput is separate).
Its final virtual callback slot corresponds to the empty body 00408460.
Remove the invented puInputCore and GuiInputObject hierarchy. The direct call
to puObject_ctor is consistent with an inlined puInput constructor, and does
not establish a distinct game-owned class. Original class spelling remains
inferred from the matching PLIB methods. This documents the executable variant
relative to 1.3.2, not an identification of an exact upstream release.

The recovered base ordering places sort_priority at 360, user_data at 400,
callbacks/render state at 404..420, and next/prev at 424/428. Correct the
misleading sort_priority offset comment. Existing 32-bit size assertions now
cover puObject=432, puGroup=440 and puInput=448 in the game factory unit.

## MG-PLIB-091 — remove upstream PUI legend-placement word

PLIB1.3.2 `puObject` stores `legendPlace` after `legendFont`, making the
32-bit class 436 bytes. The executable's `puObject` layout is 432 bytes:
`puObject_ctor` initializes the owner-element pointer at offset400 directly
after the 12-byte `legendFont`, followed by callbacks at404,408,412,416 and420,
and list links at424 and428. This matches `RecoveredGuiWidget32`. No executable
code reads a legend-placement word. Remove the field, initializer and accessors
from the vendored PUI class so subsequent fields use binary-evidenced offsets.
The excluded popup-menu source is the only remaining upstream caller of those
accessors and is outside the PUI build allowlist.

This is a confirmed PUI base-class variant relative to the PLIB1.3.2 reference,
not a pointer-size or compatibility-layer effect. With the corrected 432-byte
base, GUI type 5's four-word tail produces the executable's exact 448-byte
allocation. The separate upstream `puInput::valid_data` member remains in the
reference class; the game's manually constructed type-5 object does not
include it. In Megusta, `puInputCore` factors the four-word state and shared
input methods so both the PLIB reference `puInput` and game-side `GuiInputObject`
can use the same implementation without a cross-class pointer cast. This is an
internal source organization for the reimplementation, not a claim that the
original binary declared a class with that name.

The vtable slice at004ea870 has the `puObject` destructor/label/hit/size/bounds
slots, PLIB input methods in slots3,7,8, and the game zero-return routine in
slot9. The factory calls only `puObject_ctor`, writes one vptr at offset0, then
writes four integer state values; there is no second base-vptr initialization.
The input method entries point directly to their bodies rather than to
`this`-adjusting thunks. This strongly argues against a polymorphic secondary
base or multiple inheritance. The binary supports a compact game-owned input
role built on the primary `puObject` prefix, but cannot recover its source class
name or exact declaration.

## MG-PY-001 — CPython 1.5.2 static Win32 build compatibility

`vendor/Python-1.5.2/Megusta/config.h`, `vendor/Python-1.5.2/Megusta/builtin_config.c` and
`vendor/Python-1.5.2/Megusta/largeint.h` are local build/configuration files. The recovered
built-in table replaces stock `PC/config.c`; the selected modules are listed
in `cmake/CPython.cmake`. `manifests/python_build_scope.json` remains an audit
record and is not required to configure or build.

Two narrow edits are applied to the vendored CPython source. In
`PC/import_nt.c`, the registry value size is `LONG`, matching the Win32 API
declaration; the buffer capacity is converted from the function's `int`
argument. In `Modules/timemodule.c`, the stock `<largeint.h>` include resolves
to the Megusta compatibility subset, which uses the SDK's `LARGE_INTEGER` and
implements only the subtract/divide/zero helpers called by the preserved
performance-counter clock path. These changes adapt old MSVC assumptions to
the current Clang/MinGW SDK and do not intentionally change Python behavior.

The current target compiles the units explicitly listed in
`cmake/CPython.cmake` plus its local support files. The scene-inspection program
does not call Python, so dead stripping can remove unused interpreter objects.
This is build/link evidence only; no interpreter runtime behavior is claimed.

## MG-PY-003 — Keep UCRT timezone aliases local

For MSVC-compatible builds, `Modules/timemodule.c` maps CPython's legacy
`timezone`, `daylight` and `tzname` references to the underscored UCRT globals.
The aliases are placed after its system-header includes. Defining these macros
in the shared Python config rewrites the `timezone` member inside the Windows
SDK's `struct __timeb`, producing C2032 errors in `<sys/timeb.h>`.

## MG-PY-002 — Restore omitted runtime dependencies and disabled interrupts

The full selected-library link check exposed source omissions previously hidden
by application section collection. Added stock `Python/importdl.c` and
`Modules/getbuildinfo.c` to the allowlist. The saved `_PyImport_LoadDynamicModule`
at `0049fa40` unconditionally raises “dynamically linked modules not supported”;
compile importdl.c with its supported `NO_DYNAMIC_LINK` switch. The saved
`Py_GetBuildInfo` at `004b5190` formats BUILD=0, DATE="Aug 14 2001" and
TIME="16:17:42"; per-source definitions reproduce these values. Neither body
was changed in the vendored source.

`vendor/Python-1.5.2/Megusta/interrupt_stubs.c` implements the disabled interrupt hooks required
by the selected upstream runtime. The saved Py_Initialize/Py_Finalize interrupt
sites call a shared no-op stub; PyObject_Print and errno-handling signal checks
call a shared return-zero stub. The saved my_fgets interrupt probe similarly
uses the return-zero service. These are explicit local runtime replacements,
not a stock signal-module implementation. Body hashes and call-site evidence
are retained in `manifests/python_build_scope.json`. No OS thread backend or
signal module was added.

The separate `megusta_library_link_check` target retains every selected library
object with section collection disabled, exposing unresolved dependencies that
the ordinary scene application can discard. It now links successfully; no
runtime or original-executable parity is inferred from this check. Reproduce
and record it as described in `docs/LIBRARY_COVERAGE.md`.
# Native SDL Linux/macOS integration

The SDL window, event, timing and input services are shared by native desktop
and browser targets under `src/platform/sdl`. The patched PLIB PUI clock and
SSG context checks use SDL on these targets; legacy GLX/AGL includes are
excluded when SDL owns the context. These are platform adaptations, not
claims about the original library's architecture. Desktop rendering uses
system OpenGL 2.1; browser GLES/gl4es hooks remain separate.

Modern macOS compatibility: zlib 1.1.3 defines its `Byte` type even when
Apple SDK macros identify a Mac target, and libpng 1.0.8 uses `math.h`
instead of classic Mac OS `fp.h`. Native CPython selects POSIX configuration,
actual LP64 long/pointer sizes, and the upstream POSIX socket initializer.
See `docs/NATIVE_SDL.md` for dependencies, presets and validation limits.

