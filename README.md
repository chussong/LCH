# Library of Convenient Headers (LCH)

This is a header-only library containing general-purpose utility headers used in
Joyz projects. Most will work with C++14 or higher; some may work with C++11 
and a few may require C++17. 

Since these are headers, you generally can use them without changing how your 
program is compiled. However, file.hpp is an exception: it provides a common
interface to the \<filesystem\> header (in C++17 and later) and the similar
\<boost/filesystem\> library (in C++14 and earlier). At link time, if this 
header was included as C++14 or lower, it will require a 
`-lboost_filesystem -lboost_system`; if it was included as C++17, it will likely
require a `-lstdc++fs` on GCC (Linux) or a `-lc++fs` on clang (MacOS).

Some unit tests are available. You can build them by running `make` in the 
`tests` directory, and subsequently run them with `./lch_test`. The tests are
made using Catch2, so commands for that should work normally.

Written and maintained by [Charles Hussong](mailto:c.hussong@joyz.co.jp) 
for [Joyz Inc.](https://www.joyz.co.jp/) in Tokyo, Japan.
