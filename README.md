This is an implementation of exact chess ratings calculations, inspired by
https://github.com/jazzzooo/Lichess-Ordo/tree/master/jazzo, which was in turn
inspired by https://github.com/michiguel/Ordo

It attempts to be fast by not being quite as exact as Ordo, and using multiple
threads.

The data format is a list of games:

    black:white:result

where the result is `w`, `b`, or `d` for white, black or draw.
