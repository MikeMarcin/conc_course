// Measure the cost of associativity misses

#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
//#include <sys/mman.h>
#include <intrin.h>
#include <memory>
// #include "../../stddev.h"
#include "benchmark.hpp"

// #define GRAPH // This is the dumbest thing ever. TODO: take an argument

constexpr size_t operator "" _kib( size_t n ) { return n << 10; }
constexpr size_t operator "" _mib( size_t n ) { return n << 20; }
constexpr size_t line_size = 64;
constexpr size_t page_size = 4_kib;
constexpr size_t max_num_lines = 100'000;
constexpr size_t word_size = 8;
constexpr size_t alloc_size = 2 * max_num_lines * page_size;

// #define HUGEPAGE 1


// Do 'n' accesses with a relative offset of 'align'
// pointer_chase == 0: do reads and use the read in a running computation
// pointer_chase == 1: do read and use read to find next address
uint64_t access_mem(int align, int n, int runs, int pointer_chase) {
  //  static uint64_t a[2 * MAX_NUM_LINES * page_size];
 #ifdef HUGEPAGE
  uint64_t *a = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE| MAP_ANONYMOUS, -1, 0);
  madvise(a, alloc_size, MADV_HUGEPAGE);
#else
  //static uint64_t a[alloc_size];
#endif // HUGEPAGE
  auto a = std::make_unique< uint64_t[] >( alloc_size );


  uint64_t sum = 0; // Prevent code from being optimized away
  uint64_t tsc_before, tsc_after, tsc, min_tsc;
  uint64_t offset;

  min_tsc = UINT64_MAX;

  int i, j;
  // Fill up array with some nonsense
  for ( i = 0; i < n * page_size; i++ ) {
      a[i] = i % 17;
  }
  // Generate pointer chasing array accesses. Each a[i] that we want to access
  // points to something that's (page_size+align)/word_size down the line. The
  // last entry points to 0 so that we loop around.
  offset = 0;
  for ( i = 0; i < n * page_size; i += ( page_size + align ) / word_size ) {
      offset += ( page_size + align ) / word_size;
      a[i] = offset;
  }
  a[n * page_size / word_size] = 0;

  // Do accesses separated by one page +/- alignment offset
  for ( i = 0; i < runs; i++ ) {
      offset     = 0;
      tsc_before = read_timestamp();
      for ( j = 0; j < n; j++ ) {
          sum += a[offset];
          if ( pointer_chase ) {
              offset = a[offset];
          } else {
              offset += page_size + align;
          }
      }
      tsc_after = read_timestamp();
      tsc       = tsc_after - tsc_before;
      min_tsc   = min_tsc < tsc ? min_tsc : tsc;
  }

  DoNotOptimize( sum );

  return min_tsc;
}

void test_and_print( int n, int pointer_chase ) {
    double diff;
    uint64_t aligned_time, unaligned_time;
    int runs = 100000;


    unaligned_time = access_mem( line_size, n, runs, pointer_chase );
    aligned_time = access_mem( 0, n, runs, pointer_chase );
    diff = (double)aligned_time / (double)unaligned_time;

    printf( "----------%i accesses--------\n", n );
    printf( "Page-aligned time:         %" PRIu64 "\n", aligned_time );
    printf( "Page-unaligned (+64) time: %" PRIu64 "\n", unaligned_time );
    printf( "Difference: %f\n", diff );
}

void inefficient_csv_output(int n, int pointer_chase) {
    double diff;
    uint64_t *aligned_time, *unaligned_time;
    int runs = 100;
    int i;

    aligned_time = (uint64_t*)malloc( n * sizeof( uint64_t ) );
    unaligned_time = (uint64_t*)malloc( n * sizeof( uint64_t ) );

    for ( i = 0; i < n; i++ ) {
        aligned_time[i] = access_mem( 0, i, runs, pointer_chase );
        unaligned_time[i] = access_mem( line_size, i, runs, pointer_chase );
    }

    for ( i = 0; i < n; i++ ) {
        printf( "%" PRIu64 ",", aligned_time[i] );
    }
    printf( "\n" );

    for ( i = 0; i < n; i++ ) {
        printf( "%" PRIu64 ",", unaligned_time[i] );
    }
    printf( "\n" );

    for ( i = 0; i < n; i++ ) {
        diff = (double)aligned_time[i] / (double)unaligned_time[i];
        printf( "%f,", diff );
    }
    printf( "\n" );
}

//int main() {
//  #ifndef GRAPH
//  test_and_print(16, 1);
//  test_and_print(32, 1);
//  test_and_print(64, 1);
//  test_and_print(128, 1);
//  test_and_print(256, 1);
//  test_and_print(512, 1);
//  test_and_print(1024, 1);
//  #else
//  inefficient_csv_output(1024, 0);
//  inefficient_csv_output(1024, 1);
//  #endif
//  return 0;
//}
