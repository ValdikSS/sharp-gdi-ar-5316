Sharp GDI printing language specification
=========================================

This is a specification of what Sharp calls **GDI** in their print driver.  I call it `200z`, because of the first readable string.

The output looks like this (from Sharp AR-5316):

```
00000000  81 81 00 00 00 08 81 81  00 2d 00 02 32 30 30 7a  |.........-..200z|
00000010  30 70 30 73 36 30 30 72  30 6a 31 6f 31 78 38 30  |0p0s600r0j1o1x80|
00000020  30 30 30 30 61 30 68 31  6d 30 76 30 69 30 62 30  |0000a0h1m0v0i0b0|
00000030  74 36 30 30 64 31 63 30  65 81 81 00 2b 00 05 37  |t600d1c0e...+..7|
00000040  68 39 61 30 64 32 35 37  70 33 39 33 36 74 35 34  |h9a0d257p3936t54|
00000050  37 32 73 33 38 34 65 31  31 33 36 6c 32 6d 31 36  |72s384e1136l2m16|
00000060  76 34 38 75 30 6f 30 72  31 66 81 81 00 00 00 10  |v48u0o0r1f......|
00000070  81 81 7f fc 00 01 82 00  82 00 82 00 82 00 82 00  |................|
```

According to the driver's `sf1cjins.inf` file, this format is used for the following printers:

* SHARP AR-141G
* SHARP AR-160M
* SHARP AR-200M
* SHARP AR-1118
* SHARP AR-2818
* SHARP AR-7818
* SHARP AR-M160
* SHARP AR-M205
* SHARP AR-5316
* SHARP AR-5220

There's also **SPLC** language, which output begins with `201z`. This is a different language which is not described here
(however it also contains the described here raster data, but only with "you've chosen the wrong driver" message if the printer can't decode `201z` raster).

## Format overview

Every packet starts with a 6-byte header followed by an optional body:

`81 81  LL LL  TT TT  [LL bytes of body]`

Where

* Bytes 0-1: **0x81 0x81** — packet magic
* Bytes 2-3: big-endian uint16 LL — body (payload) length in bytes
* Bytes 4-5: big-endian uint16 TT — packet type / command
* Bytes 6..: LL bytes of body — absent when LL == 0

When LL == 0x0000: TT is a command code; no body follows (6 bytes total).
When LL != 0x0000: TT is a data type; LL bytes of body follow.

## Commands

* 0x0008  (8) – Begin job (first packet in file)
    - `81 81 00 00 00 08`

* 0x0002 (2) — Begin job description
    - `81 81 nn nn 00 02 <nn nn payload bytes, ascii>`

* 0x0005 (5) — Begin page description
    - `81 81 nn nn 00 05 <nn nn payload bytes, ascii>`

* 0x0010  (16) — Begin page/raster (precedes each page's image bands)
    - `81 81 00 00 00 10`

* 0x0001 (1) — Raster data, up to 32764 (7F FC) bytes in size.
    - `81 81 nn nn 00 01 <nn nn payload bytes>`

* 0x0006  (6) – End raster/page
    - `81 81 00 00 00 06`

* 0x0003  (3)   – End job
    - `81 81 00 00 00 03`

* 0x0009  (9)   – "I'm done" (last packet in file)
    - `81 81 00 00 00 09`

## Command payloads

#### 0x0002 – Job configuration string (ASCII key=value pairs)

Format: tokens are `<value><key_letter>`, all concatenated.  
Example: `200z0p0s600r0j1o1x800000a0h1m0v0i0b0t600d1c0e`  
Known keys:
* **200z** — output/driver type
* **r**  – print resolution in dpi  (300 = draft, 600 = normal/photo)
* **c**  – collation: 1=collate (default), 0=no collate

Other keys appear to be fixed/unknown. They are mostly used in SPLC driver, but not GDI.

#### 0x0005  – Page configuration string (ASCII key=value pairs)

Format: same `<value><key_letter>` encoding.  
Example: `7h9a0d257p4768t6816s96e96l2m16v48u0o0r1f`

Keys decoded (value precedes letter):

* [num]h — Paper tray (FIRST occurrence of 'h' in the string):
    - 7   = auto / default cassette
    - 257 = Tray 1 (front cassette)
    - 261 = Manual / bypass tray

* [num]a  – Paper size code:
    - 1 = US Letter
    - 2 = US Ledger
    - 5 = US Legal
    - 6 = Statement
    - 7 = Executive
    - 8  = A3
    - 9  = A4
    - 11 = A5
    - 12 = JIS B4
    - 13 = JIS B5
    - 14 = US Legal (Alternate / Folio)
    - 20 = Com-10 Envelope
    - 27 = DL
    - 28 = C5
    - 43 = Hagaki
    - 69 = Ofuku Hagaki
    - 70 = A6
    - 73 = JIS Chou #3 Envelope
    - 88 = JIS B6
    - 93 = PRC 16K
    - 280 = PRC 32K
    - 281 = Chinese Envelope #8

* [num]s  – Raster WIDTH  in pixels  ← use for image decoding
* [num]t  – Raster HEIGHT in pixels  ← use for image decoding

* [num]o  – User-selected print orientation:
    - 0 = portrait
    - 1 = landscape

Landscape does NOT change raster dimensions — the raster
content is simply rotated 180° relative to portrait.

* [num]r  – Always equals o (mirrors the user orientation flag).

* [num]f  – Raster bitmap physical layout:
    - 0 = portrait raster  (t > s, rows are page-width)
    - 1 = landscape raster (s > t, rows are page-height; rotate 270° CCW to view upright)

Observed:  
A3 paper:              always f=0 (portrait raster)  
A4/A5 auto or Tray 1:  always f=1 (landscape raster)  
A4/A5 manual/bypass:   f=0       (portrait raster, like A3)

* [num]e  – Top/right printable margin in pixels
* [num]l  – Bottom/left printable margin in pixels
    - Observed #1: asymmetric (e=192, l=96 for normal A4)
    - Observed #2: symmetric  (e=96,  l=96 for normal A4)

* [num]p  – Unknown
* [num]u  – Always 48 (fixed constant, possibly driver/protocol version)
* [num]m  – Always 16 (unknown)
* [num]v  – Always 48 (unknown)
* [num]d  – Always 0  (unknown)

#### 0x0001  – Image data band

See "Image stream" section below.

## Multi-page jobs

Each page in a multi-page job is bracketed by:  
  end-raster (0x0006) - begin-page-description (0x0005) - begin-raster (0x0010) - [0x0001 image bands] - end-raster (0x0006)

For multi-copy jobs:  
  Collated  (d=1 in JOB):  pages sent as `p1 p2 p3 p4 | p1 p2 p3 p4`  
  Uncollated(d=0 in JOB):  pages sent as `p1 p1 | p2 p2 | p3 p3 | p4 p4`

## Image stream

The printer driver produces one continuous 1-bit monochrome PackBits-compressed byte-stream
per page raster. This stream is split into chunks of up to 32764 (7F FC) bytes,
and transported across successive 0x0001 packets between begin-raster and end-raster.

`[2B magic 81 81] [2B LL=0x7FFC] [2B type 00 01] [32764B PackBits body]`

The PackBits stream is cut at arbitrary byte positions with no alignment to
raster row boundaries or PackBits token boundaries. A single PackBits token
may straddle a packet boundary.

## Compression

Apple's [PackBits](https://en.wikipedia.org/wiki/PackBits) is used for compression.

* 0x00 - 0x7F (0 - 127): Write the next (n + 1) bytes verbatim
* 0x81 - 0xFF (129 - 255) RLE: repeat the next single byte (257 - n) times
* 0x80 (128) No-op, not used

PackBits is a very simple algorithm, without lookback/lookahead.  
However, because the stream is continuous, an RLE run of say 0x00 bytes can span multiple raster rows. It doesn't have to terminate on a line boundary.
