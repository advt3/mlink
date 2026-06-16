#include <zephyr/ztest.h>
#include "filter.hpp"

ZTEST(filter_tests, test_initial_reading)
{
    ReadingFilter filter(10000); // 10s interval
    SensorReading r1{
        .temperature = 20.0,
        .humidity = 50.0,
        .pressure = 1000.0,
        .address = 0x77
    };
    
    filter.add_sample(r1);

    SensorReading out;
    // Initial reading should always be published if sample count > 0
    zassert_true(filter.check_and_get_reading(out, 0), "Initial reading must be published");
    zassert_equal(out.temperature, 20.0, "Averaged temperature should match input");
    zassert_equal(out.humidity, 50.0, "Averaged humidity should match input");
    zassert_equal(out.pressure, 1000.0, "Averaged pressure should match input");
}

ZTEST(filter_tests, test_interval_threshold)
{
    ReadingFilter filter(10000); // 10s interval
    SensorReading r1{
        .temperature = 20.0,
        .humidity = 50.0,
        .pressure = 1000.0,
        .address = 0x77
    };
    
    filter.add_sample(r1);
    SensorReading out;
    filter.check_and_get_reading(out, 0); // Publish initial

    // Add another sample
    filter.add_sample(r1);

    // Before interval has elapsed (e.g. 5000ms), check_and_get_reading should return false
    zassert_false(filter.check_and_get_reading(out, 5000), "Should not publish before interval elapsed");

    // After interval has elapsed (e.g. 10000ms), check_and_get_reading should return true
    zassert_true(filter.check_and_get_reading(out, 10000), "Should publish after interval elapsed");
}

ZTEST(filter_tests, test_averaging_logic)
{
    ReadingFilter filter(10000); // 10s interval
    SensorReading r1{
        .temperature = 20.0,
        .humidity = 50.0,
        .pressure = 1000.0,
        .address = 0x77
    };
    
    filter.add_sample(r1);
    SensorReading out;
    filter.check_and_get_reading(out, 0); // Publish initial

    // Add two different samples
    SensorReading r2{
        .temperature = 22.0,
        .humidity = 60.0,
        .pressure = 1010.0,
        .address = 0x77
    };
    SensorReading r3{
        .temperature = 24.0,
        .humidity = 70.0,
        .pressure = 1020.0,
        .address = 0x77
    };
    filter.add_sample(r2);
    filter.add_sample(r3);

    zassert_true(filter.check_and_get_reading(out, 10000), "Should publish after interval elapsed");
    zassert_equal(out.temperature, 23.0, "Average temperature (22 + 24)/2 = 23");
    zassert_equal(out.humidity, 65.0, "Average humidity (60 + 70)/2 = 65");
    zassert_equal(out.pressure, 1015.0, "Average pressure (1010 + 1020)/2 = 1015");
}

ZTEST_SUITE(filter_tests, NULL, NULL, NULL, NULL, NULL);
