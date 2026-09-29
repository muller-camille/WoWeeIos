Stand-ins for the Apple system headers metal-cpp includes, so that
`../check.sh` can parse the Metal build's C++ on Linux. Only as much of each
as metal-cpp and this renderer use - types, a few declarations, no
definitions that do anything. They are never compiled into anything that
runs.
