#ifndef TESTTHAT_HPP
#define TESTTHAT_HPP

#define CATCH_CONFIG_PREFIX_ALL
#define CATCH_CONFIG_NOSTDOUT

#ifdef TESTTHAT_TEST_RUNNER
# define CATCH_CONFIG_RUNNER
#endif

// Catch has calls to 'exit' on failure, which upset R CMD check.
// We won't bump into them during normal test execution so just temporarily
// hide it when we include 'catch'. Make sure we get 'exit' from the normal
// places first, though.
#include <cstddef> // std::size_t
#include <cstdlib> // exit
#include <cstdio>  // EOF
extern "C" inline void testthat_exit_override(int status) throw() {}

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wparentheses"
#endif

#define exit testthat_exit_override
#include "vendor/catch.h"
#undef exit

// Implement an output stream that avoids writing to stdout / stderr.
extern "C" void Rprintf(const char*, ...);
extern "C" void R_FlushConsole();

namespace testthat {

class r_streambuf : public std::streambuf {
public:

  r_streambuf() {}

protected:

  virtual std::streamsize xsputn(const char* s, std::streamsize n) {
    Rprintf("%.*s", n, s);
    return n;
  }

  virtual int overflow(int c = EOF) {
    if (c != EOF) Rprintf("%.1s", &c);
    return c;
  }

  virtual int sync() {
    R_FlushConsole();
    return 0;
  }

};

class r_ostream : public std::ostream {

public:

  r_ostream() :
    std::ostream(new r_streambuf), pBuffer(static_cast<r_streambuf*>(rdbuf()))
  {}

private:
  r_streambuf* pBuffer;

};

// Allow client packages to access the Catch::Session
// exported by testthat.
#ifdef CATCH_CONFIG_RUNNER

Catch::Session& catchSession()
{
  static Catch::Session instance;
  return instance;
}

inline bool run_tests()
{
  return catchSession().run() == 0;
}

#endif

} // namespace testthat

namespace Catch {

inline std::ostream& cout()
{
  static testthat::r_ostream instance;
  return instance;
}

inline std::ostream& cerr()
{
  static testthat::r_ostream instance;
  return instance;
}

} // namespace Catch

#define context(__X__) CATCH_TEST_CASE(__X__ " | " __FILE__)
#define test_that CATCH_SECTION
#define expect_true CATCH_CHECK
#define expect_false CATCH_CHECK_FALSE

#ifdef TESTTHAT_TEST_RUNNER

// ERROR will be redefined by R; avoid compiler warnings
#ifdef ERROR
# undef ERROR
#endif

#include <R.h>
#include <Rinternals.h>
extern "C" SEXP run_testthat_tests() {
  bool success = testthat::run_tests();
  return ScalarLogical(success);
}
#endif

#endif

