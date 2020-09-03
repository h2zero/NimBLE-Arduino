# Changelog

All notable changes to this project will be documented in this file.  

## [1.0.1] - 2020-09-02

### Added

- Empty `NimBLEAddress` constructor: `NimBLEAddress()` produces an address of 00:00:00:00:00:00 type 0.
- Documentation of the difference of NimBLEAddress::getNative vs the original bluedroid library.

### Changed

- notify_callback typedef is now defined as std::function to enable the use of std::bind to call a class member function.

### Fixed

- Fix advertising start delay when first called.


## [1.0.0] - 2020-08-22

First stable release.

All the original library functionality is complete and many extras added with full documentation.
