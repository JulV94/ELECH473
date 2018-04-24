#!/bin/bash

convert -size 1024x1024 -depth 8 gray:test_min_max_3_c.raw out_min_max_3_c.bmp
convert -size 1024x1024 -depth 8 gray:test_min_max_3_simd.raw out_min_max_3_simd.bmp

convert -size 1024x1024 -depth 8 gray:test_min_max_5_c.raw out_min_max_5_c.bmp
convert -size 1024x1024 -depth 8 gray:test_min_max_5_simd.raw out_min_max_5_simd.bmp

convert -size 1024x1024 -depth 8 gray:test_min_max_7_c.raw out_min_max_7_c.bmp
convert -size 1024x1024 -depth 8 gray:test_min_max_7_simd.raw out_min_max_7_simd.bmp
