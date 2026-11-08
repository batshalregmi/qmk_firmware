# Toggle feedback regression test

Run from the repository root (requires a C compiler):

```sh
cc -std=c11 -Wall -Wextra -Wno-unused-parameter \
  -I tools/toggle_feedback_test tools/toggle_feedback_test/test.c \
  -o /tmp/v6-feedback-test
/tmp/v6-feedback-test
```

The harness compiles the actual feedback/dial source and pinned getreuer
module with stubbed timers, RGB hardware, mouse reports and deferred execution.
It checks lock parity over 4,000 randomized tap/configuration sequences, timer
rollover, both mouse buttons, neutral, rate limits, stopping before a button
change, and held-button release pairing. Dial events must never emit new clicks.
It also checks confirmation colors, preview/flash priority, unchanged lighting when Fn alone is held,
persistent lock indication, RGB-off restoration, and suspend cancellation.

It does not simulate physical LEDs or USB delivery; verify the appearance and
click behavior on the keyboard after flashing.
