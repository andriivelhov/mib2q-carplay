# mib2q-carplay vfix builds (andriivelhov fork)

Test builds on top of the official **v1.3.1** release. Only the two video programs change
(`libaltscreen111_mhi2q.so`, `altscreen_render`); the Java patch, installer and menu are the
v1.3.1 files, byte for byte. Rebuilding v1.3.1 with the same toolchain reproduces the official
binaries exactly, so every difference below comes from the listed commits.

Install: copy the release folder to the card as `carplay/` and press
**Green Engineering Menu -> CarPlay-RGI -> Install / update CarPlay**, then a normal reboot.
Roll back: the same with the official v1.3.1 `carplay/` folder.

Test car: 2018 Audi A5 Sportback B9, MHI2Q_US_AUG22_P5087 / MU1316, wired CarPlay, iPhone on
iOS 27. Upstream report: [kamgurgul/mib2q-carplay#39](https://github.com/kamgurgul/mib2q-carplay/issues/39).

## 1.3.1-vfix2 (in test)

Branch `car/v1.3.1-vfix2-perf`.

- **The hardware decoder's window post no longer waits for the compositor.** Each picture
  waited in `screen_post_window(..., SCREEN_WAIT_IDLE)` for a whole composition, which held the
  decoder's output thread. During fast map rotation (frames of about 45 KB) the hook queue backed
  up and skipped to the next IDR. That happened 32 times in car log 004 and showed as visible
  jumps. The decoder buffer is still safe to hand back: `screen_flush_blits(SCREEN_WAIT_IDLE)`
  waits for the blit only.
- **Window 99 gets 3 buffers** (shown, queued, being blitted). If the car refuses 3, it falls
  back to 2.
- **Timing stats every 600 pictures** in `altscreen_render.log`: blit and post average/maximum,
  and input-buffer waits over 50 ms.
- **The old behaviour can be restored without reinstalling.** Create
  `/mnt/app/root/altscreen_render.postwait`; it takes effect the next time the decoder opens.

## 1.3.1-vfix1 (stable, tag `v1.3.1-vfix1`)

Road-tested 2026-10-28/29. In car log 004 there were 32 queue overflows; all recovered in
126–295 ms, with 0 consumer restarts and 0 fallbacks to the Audi map. Before this build there
were 109 restarts in 23 minutes.

- **No drop-outs while the phone is connected.** The CarPlay cluster map stays up for as long as
  the tee is connected (live timeout 0, as Android Auto already had). Apple Maps without a route
  pauses for seconds at a time, and the old 500 ms timeout dropped to the Audi map.
- **Queue overflow skips to the next IDR** instead of restarting the consumer:
  - only unsent pictures are dropped; a partly written one is kept;
  - the hook requests a keyframe and retries every 1 s;
  - after 5 s without an IDR it falls back to the old restart.
- **Queue raised from 16 to 64 pictures.** The 4 MB byte cap still applies.
- **Each picture is shown as soon as it arrives.** An access unit delimiter is appended after
  every picture, so the renderer's H.264 parser no longer waits for the next picture first.
- **Hardware-decoded pictures are counted on the 200 ms idle tick.**
- **Faster tee socket:** `TCP_NODELAY` and 256 KB buffers.
