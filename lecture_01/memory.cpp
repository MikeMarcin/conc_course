#include <random>
#include <memory>
#include <numeric>
#include <benchmark/benchmark.h>

constexpr size_t operator "" _kib( size_t n ) { return n << 10; }
constexpr size_t operator "" _mib( size_t n ) { return n << 20; }

constexpr size_t line_size = 64;
constexpr size_t page_size = 4_kib;
constexpr size_t max_num_lines = 100'000;
constexpr size_t word_size = 8;

constexpr size_t element_size = 1;
constexpr size_t padding_size = 0;
//constexpr size_t working_set_size = 1_mib;

constexpr size_t bigger_than_cachesize = 10_mib;

struct clear_cache {
    clear_cache() : p( new char[bigger_than_cachesize] ) {}
    std::unique_ptr<char[]> p;

    void clear() {
        // When you want to "flush" cache. 
        for ( int i = 0; i < bigger_than_cachesize; i++ ) {
            p[i] = 0;
        }
        benchmark::ClobberMemory();
    }
} g_cache_clearer;


void clear_data_cache() {
    g_cache_clearer.clear();
}

// test how many streams of predictions we can have running
// test memory pattern matching
// test aligned vs unaligned data
// test page boundaries prefetcher
// test dependent memory reads (pointer chasing)
// test sequential/random/revese access
// test page-aligned vs non-page aligned

template< int SizeInBytes >
struct element {
    static_assert(SizeInBytes % 8 == 0, "SizeInBytes must be a multiple of 8");
    static constexpr int NPad = (SizeInBytes / 8)-1;
    int64_t next;
    uint64_t pad[NPad];

    // for iota
    element& operator++() {
        ++next;
        return *this;
    }
};

template<>
struct element<8> {
    int64_t next;

    // for iota
    element& operator++() {
        ++next;
        return *this;
    }
};

struct memory_layout_ {};

struct memory_layout_contiguous {};

enum class access_type {
    sequential,
    reverse,
    random,
};

template< typename element_type >
__forceinline void memory_read_loop( element_type * data, const int num_elements )
{
    int64_t next = 0;
    for ( int i = 0; i < num_elements; ++i ) {
        auto & v = data[next];
        next = v.next;
        benchmark::DoNotOptimize( next );
    }
}

template< typename element_type >
void BM_memory_read( benchmark::State& state ) {
    
    const int working_set_log2 = state.range( 0 );
    const int working_set_size = 1 << working_set_log2;
    access_type access = static_cast<access_type>(state.range( 1 ));
    const int num_elements = working_set_size / sizeof(element_type);
    // const int y = state.range(1);
    std::unique_ptr<element_type[]> data{ new element_type[num_elements] };
    std::iota( data.get(), data.get() + num_elements, element_type{0} );
    switch ( access ) {
        case access_type::sequential: {
        }
        break;
        case access_type::reverse: {
            std::reverse( data.get(), data.get() + num_elements );
        }
        break;
        case access_type::random: {
            const auto seed_val = std::mt19937::default_seed;
            std::mt19937 rng( seed_val );
            std::shuffle( data.get(), data.get() + num_elements, rng );
        }
        break;
    }

    clear_data_cache();

    // run the loop once to warm the cache
    // this is the best option we have to keep all iterations identical
    memory_read_loop( data.get(), num_elements );

    for ( auto _ : state ) {
        memory_read_loop( data.get(), num_elements );
    }
    state.counters.insert( {"element_count", num_elements} );
    state.counters.insert( {"element_size", static_cast<double>(sizeof(element_type))} );
    state.counters.insert( {"access_type", static_cast<double>(access)} );
    state.counters.insert( {"working_set", working_set_log2 } );
}

static void MemoryReadArgs( benchmark::internal::Benchmark* b ) {
    for ( int j = 0; j < 3; ++j ) {
        // sequential, reverse, random
        for ( int i = 10; i <= 28; ++i ) {
            b->Args( { i, j } );
        }
    }
}

//BENCHMARK_TEMPLATE( BM_memory_read, element<8> )->Apply( MemoryReadArgs );
//BENCHMARK_TEMPLATE( BM_memory_read, element<64> )->Apply( MemoryReadArgs );
//BENCHMARK_TEMPLATE( BM_memory_read, element<128> )->Apply( MemoryReadArgs );
//BENCHMARK_TEMPLATE( BM_memory_read, element<256> )->Apply( MemoryReadArgs );
enum class align_type {
    aligned,
    unaligned,
};

void BM_cache_conflict( benchmark::State& state ) {
    // adapted from https://danluu.com/3c-conflict/
    const int working_set_log2 = state.range( 0 );
    const int n = 1 << working_set_log2;
    const int pointer_chase = 1; // state.range( 2 );
    align_type alignType = static_cast<align_type>(state.range( 1 ));
    const int align = alignType == align_type::unaligned ? static_cast<int>(line_size) : 0;

    // Do 'n' accesses with a relative offset of 'align'
    // pointer_chase == 0: do reads and use the read in a running computation
    // pointer_chase == 1: do read and use read to find next address
    
    //  static uint64_t a[2 * MAX_NUM_LINES * page_size];

    constexpr size_t alloc_size = 2 * max_num_lines * page_size;
    auto a = std::make_unique< uint64_t[] >( alloc_size );

    uint64_t offset;

    int i, j;
    // Fill up array with some nonsense
    for ( i = 0; i < n * page_size; i++ ) {
        a[i] = i % 17;
    }
    // Generate pointer chasing array accesses. Each a[i] that we want to access
    // points to something that's (page_size+align)/word_size down the line. The
    // last entry points to 0 so that we loop around.
    offset = 0;
    for ( i = 0; i < n * page_size; i += (page_size + align) / word_size ) {
        offset += (page_size + align) / word_size;
        a[i] = offset;
    }
    a[n * page_size / word_size] = 0;

    // Do accesses separated by one page +/- alignment offset
    for ( auto _ : state ) {
        offset = 0;
        for ( j = 0; j < n; j++ ) {
            auto next = a[offset];
            benchmark::DoNotOptimize( next );
            if ( pointer_chase ) {
                offset = next;
            }
            else {
                offset += page_size + align;
            }
        }
    }    

    state.counters.insert( { "working_set", working_set_log2 } );
    state.counters.insert( { "align_type", static_cast<int>(alignType) } );
}

static void CacheConflictArgs( benchmark::internal::Benchmark* b ) {
    for ( int j : {1, 0} ) {  // aligned / unaligned
        for ( int i = 0; i <= 10; ++i ) {
            b->Args( { i, j } );
        }
    }
}
BENCHMARK( BM_cache_conflict )->Apply( CacheConflictArgs );

#include <windows.h>
struct pin_to_core {
    pin_to_core()
    {
        HANDLE process = GetCurrentProcess();
        DWORD_PTR processAffinityMask = 1 << 2;

        BOOL success = SetProcessAffinityMask( process, processAffinityMask );
    }
} g_core_pinner;

BENCHMARK_MAIN()