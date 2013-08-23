# Wave Terrain

(Note: this is an archival project that I'm toying with the idea of
modernizing a little bit. Expect it to be old, weird, sloppily coded
and buggy.)

Generate and control noise using a joystick. The noise is generated
with wave terrain synthesis and genetic programming (a la an older
experiment, [Madsyn](https://github.com/graue/madsyn), using terrains
typed out as S-expressions (Lisp-style) as a starting point.

The idea sounds way cool, doesn't it? But in practice it's hard to
control and tends to make minor variations on the same
ambulance-siren-y sound no matter what you do. Nevertheless, I played
a show using this on Friday, February 13, 2009, the debut of my noise
band [Extremities](http://extremitiesnoise.bandcamp.com/). I
apparently wrote the whole thing during a 7-hour period prior to
playing the show.

This program also generates patterned visuals that are kind of more
interesting than the audio.

## To use

1. Install SDL libs (libsdl1.2-dev on Debian/Ubuntu)
2. Run ./build
3. Hook up a USB joystick
4. Run ./terrain
5. Mess around with the joystick

Very important: press joystick buttons 5 and 6 together to quit. If
you try pressing Escape, Q, Control-C, etc, you are *not* going to
make it quit.

## Status

This was originally written sloppily to only support OpenBSD's audio
interface. I've hacked up the code to get it to compile and sorta work
on Linux, but the audio cuts out a lot, so there's more work to be
done. If it compiles/works as is on OS X or Windows, I would be
thrilled to hear of it.

## License

I release this software to the public domain (and/or license it) under
the [Creative Commons CC0 1.0 Public Domain
Dedication](https://creativecommons.org/publicdomain/zero/1.0/).
