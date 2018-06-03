#include "hicommon.h"
#include <hi_sns_ctrl.h>
#include <jv_common.h>
#include "3518_isp.h"
#include "hi_spi.h"
#include "memmap.h"



#include <mpi_ae.h>
#include <mpi_awb.h>

#define I2C_16BIT_REG   0x0709  /* 16BIT REG WIDTH */
#define I2C_16BIT_DATA  0x070a  /* 16BIT DATA WIDTH */
#define I2C_SLAVE_FORCE 0x0706 
#define DEFAULT_MD_LEN 128
#define AR0237DC     0  /*to distinguish between ar0237DC and ar0237MIPI*/



const HI_U16 IMX178_default_gamma[] = 
{   
	0, 180, 320, 426, 516, 590, 660, 730, 786, 844, 896, 946, 994, 1040, 1090, 1130, 1170, 1210, 1248,    1296, 1336, 1372, 1416, 1452, 1486, 1516, 1546, 1580, 1616, 1652, 1678, 1714, 1742, 1776, 1798, 1830,    1862, 1886, 1912, 1940, 1968, 1992, 2010, 2038, 2062, 2090, 2114, 2134, 2158, 2178, 2202, 2222, 2246,    2266, 2282, 2300, 2324, 2344, 2360, 2372, 2390, 2406, 2422, 2438, 2458, 2478, 2494, 2510, 2526, 2546,    2562, 2582, 2598, 2614, 2630, 2648, 2660, 2670, 2682, 2698, \
	2710, 2724, 2736, 2752, 2764, 2780, 2792,    2808, 2820, 2836, 2848, 2864, 2876, 2888, 2896, 2908, 2920, 2928, 2940, 2948, 2960, 2972, 2984, 2992,    3004, 3014, 3028, 3036, 3048, 3056, 3068, 3080, 3088, 3100, 3110, 3120, 3128, 3140, 3148, 3160, 3168,    3174, 3182, 3190, 3202, 3210, 3218, 3228, 3240, 3256, 3266, 3276, 3288, 3300, 3306, 3318, 3326, 3334,    3342, 3350, 3360, 3370, 3378, 3386, 3394, 3398, 3406, 3414, 3422, 3426, 3436, 3444, 3454, 3466, 3476,    3486, 3498, 3502, 3510, 3518, 3526, 3530, 3538, 3546, \
	3554, 3558, 3564, 3570, 3574, 3582, 3590, 3598,    3604, 3610, 3618, 3628, 3634, 3640, 3644, 3652, 3656, 3664, 3670, 3678, 3688, 3696, 3700, 3708, 3712,    3716, 3722, 3730, 3736, 3740, 3748, 3752, 3756, 3760, 3766, 3774, 3778, 3786, 3790, 3800, 3808, 3812,    3816, 3824, 3830, 3832, 3842, 3846, 3850, 3854, 3858, 3862, 3864, 3870, 3874, 3878, 3882, 3888, 3894,    3900, 3908, 3912, 3918, 3924, 3928, 3934, 3940, 3946, 3952, 3958, 3966, 3974, 3978, 3982, 3986, 3990,    3994, 4002, 4006, 4010, 4018, 4022, 4032, 4038, \
	4046, 4050, 4056, 4062, 4072, 4076, 4084, 4090, 4095
};

const HI_U16 Infrared_default_gamma[]=
{ 
	0, 120, 220, 320, 416, 512, 592, 664, 736, 808, 880, 944, 1004, 1062, 1124, 1174,    1226, 1276, 1328, 1380, 1432, 1472, 1516, 1556, 1596, 1636, 1680, 1720, 1756, 1792,    1828, 1864, 1896, 1932, 1968, 2004, 2032, 2056, 2082, 2110, 2138, 2162, 2190, 2218,    2242, 2270, 2294, 2314, 2338, 2358, 2382, 2402, 2426, 2446, 2470, 2490, 2514, 2534,    2550, 2570, 2586, 2606, 2622, 2638, 2658, 2674, 2694, 2710, 2726, 2746, 2762, 2782,    2798, 2814, 2826, 2842, 2854, 2870, 2882,\
	2898, 2910, 2924, 2936, 2952, 2964, 2980,    2992, 3008, 3020, 3036, 3048, 3064, 3076, 3088, 3096, 3108, 3120, 3128, 3140, 3152,    3160, 3172, 3184, 3192, 3204, 3216, 3224, 3236, 3248, 3256, 3268, 3280, 3288, 3300,    3312, 3320, 3332, 3340, 3348, 3360, 3368, 3374, 3382, 3390, 3402, 3410, 3418, 3426,    3434, 3446, 3454, 3462, 3470, 3478, 3486, 3498, 3506, 3514, 3522, 3530, 3542, 3550,    3558, 3566, 3574, 3578, 3586, 3594, 3602, 3606, 3614, 3622, 3630, 3634, 3642, 3650,    3658, 3662, 3670, 3678, 3686, \
	3690, 3698, 3706, 3710, 3718, 3722, 3726, 3734, 3738,    3742, 3750, 3754, 3760, 3764, 3768, 3776, 3780, 3784, 3792, 3796, 3800, 3804, 3812,    3816, 3820, 3824, 3832, 3836, 3840, 3844, 3848, 3856, 3860, 3864, 3868, 3872, 3876,    3880, 3884, 3892, 3896, 3900, 3904, 3908, 3912, 3916, 3920, 3924, 3928, 3932, 3936,    3940, 3944, 3948, 3952, 3956, 3960, 3964, 3968, 3972, 3972, 3976, 3980, 3984, 3988,    3992, 3996, 4000, 4004, 4008, 4012, 4016, 4020, 4024, 4028, 4032, 4032, 4036, 4040,    4044, 4048, 4052,\
	4056, 4056, 4060, 4064, 4068, 4072, 4072, 4076, 4080, 4084, 4086, 4088, 4092, 4095
}; 
const HI_U16 FSWDR_l_gamma[]   =     
{  
	0,   1,   2,   4,   8,  12,  17,  23,  30,  38,  47,  57,  68,  79,  92, 105, 120, 133, 147, 161,        176, 192, 209, 226, 243, 260, 278, 296, 315, 333, 351, 370, 390, 410, 431, 453, 474, 494, 515,        536, 558, 580, 602, 623, 644, 665, 686, 708, 730, 751, 773, 795, 818, 840, 862, 884, 907, 929,        951, 974, 998,1024,1051,1073,1096,1117,1139,1159,1181,1202,1223,1243,1261,1275,1293,1313,        1332,1351,1371,1389,1408,1427,1446,1464,1482,1499,1516,1533,1549,1567,\
	1583,1600,1616,1633,        1650,1667,1683,1700,1716,1732,1749,1766,1782,1798,1815,1831,1847,1863,1880,1896,1912,1928,        1945,1961,1977,1993,2009,2025,2041,2057,2073,2089,2104,2121,2137,2153,2168,2184,2200,2216,        2231,2248,2263,2278,2294,2310,2326,2341,2357,2373,2388,2403,2419,2434,2450,2466,2481,2496,        2512,2527,2543,2558,2573,2589,2604,2619,2635,2650,2665,2680,2696,2711,2726,2741,2757,2771,        2787,2801,2817,2832,2847,2862,2877,2892,2907,2922,2937,2952,\
	2967,2982,2997,3012,3027,3041,        3057,3071,3086,3101,3116,3130,3145,3160,3175,3190,3204,3219,3234,3248,3263,3278,3293,3307,        3322,3337,3351,3365,3380,3394,3409,3424,3438,3453,3468,3482,3497,3511,3525,3540,3554,3569,        3584,3598,3612,3626,3641,3655,3670,3684,3699,3713,3727,3742,3756,3770,3784,3799,3813,3827,        3841,3856,3870,3884,3898,3912,3927,3941,3955,3969,3983,3997,4011,4026,4039,4054,4068,4082,        4095    
};
const HI_U16 FSWDR_h_gamma[]   =     
{  
	0,1,2,4,8,12,17,23,30,38,47,57,68,79,92,105,120,133,147,161,176,192,209,226,243,260,278,296,        317,340,365,390,416,440,466,491,517,538,561,584,607,631,656,680,705,730,756,784,812,835,        858,882,908,934,958,982,1008,1036,1064,1092,1119,1143,1167,1192,1218,1243,1269,1294,1320,        1346,1372,1398,1424,1450,1476,1502,1528,1554,1580,1607,1633,1658,1684,1710,1735,1761,1786,        1811,1836,1860,1884,1908,1932,1956,1979,2002,2024,2046,2068,2090,2112,2133,2154,2175,2196,\
	2217,2237,2258,2278,2298,2318,2337,2357,2376,2395,2414,2433,2451,2469,2488,2505,2523,2541,        2558,2575,2592,2609,2626,2642,2658,2674,2690,2705,2720,2735,2750,2765,2779,2793,2807,2821,        2835,2848,2861,2874,2887,2900,2913,2925,2937,2950,2962,2974,2986,2998,3009,3021,3033,3044,        3056,3067,3078,3088,3099,3109,3119,3129,3139,3148,3158,3168,3177,3187,3197,3207,3217,3227,        3238,3248,3259,3270,3281,3292,3303,3313,3324,3335,3346,3357,3368,3379,3389,3400,3410,3421, \
	3431,3441,3451,3461,3471,3481,3491,3501,3511,3521,3531,3541,3552,3562,3572,3583,3593,3604,        3615,3625,3636,3646,3657,3668,3679,3689,3700,3711,3721,3732,3743,3753,3764,3774,3784,3795,        3805,3816,3826,3837,3847,3858,3869,3880,3891,3902,3913,3925,3937,3949,3961,3973,3985,3997,        4009,4022,4034,4046,4058,4071,4083,4095    
};
const HI_U16 u16Gamma_wdr_h[257] = 
{
	0,0,1,2,3,5,8,10,14,17,21,26,30,36,41,47,54,61,68,75,83,92,100,109,119,129,139,150,161,173,184,196,209,222,235,248,262,276,290,305,320,335,351,366,382,399,415,433,450,467,484,502,520,539,557,576,595,614,634,653,673,693,714,734,754,775,796,816,837,858,879,901,923,944,966,988,1010,1032,1054,1076,1098,1120,1142,1165,1188,1210,1232,1255,1278,1301,1324,1346,1369,1391,1414,1437,1460,1483,1505,1528,1551,1574,1597,1619,1642,1665,1687,1710,1732,1755,1777,1799,1822,1845,1867,1889,1911,1933,\
	1955,1977,1999,2021,2043,2064,2086,2108,2129,2150,2172,2193,2214,2236,2256,2277,2298,2319,2340,2360,2380,2401,2421,2441,2461,2481,2501,2521,2541,2560,2580,2599,2618,2637,2656,2675,2694,2713,2732,2750,2769,2787,2805,2823,2841,2859,2877,2895,2912,2929,2947,2964,2982,2999,3015,3032,3049,3066,3082,3099,3115,3131,3147,3164,3179,3195,3211,3227,3242,3258,3273,3288,3303,3318,3333,3348,3362,3377,3392,3406,3420,3434,3448,3462,3477,3490,3504,3517,3531,3544,3558,3571,3584,3597,3611,3623,3636,\
	3649,3662,3674,3686,3698,3711,3723,3736,3748,3759,3771,3783,3795,3806,3818,3829,3841,3852,3863,3874,3885,3896,3907,3918,3929,3939,3949,3961,3971,3981,3991,4001,4012,4022,4032,4042,4051,4061,4071,4081,4090,4095
};
const HI_U16 u16Gamma_wdr_l[257] = 
{
	0,0,2,4,8,12,17,23,30,38,47,57,68,79,92,105,120,133,147,161,176,192,209,226,243,260,278,296,315,333,351,370,390,410,431,453,474,494,515,536,558,580,602,623,644,665,686,708,730,751,773,795,818,840,862,884,907,929,951,974,998,1024,1051,1073,1096,1117,1139,1159,1181,1202,1223,1243,1261,1275,1293,1313,1332,1351,1371,1389,1408,1427,1446,1464,1482,1499,1516,1533,1549,1567,1583,1600,1616,1633,1650,1667,1683,1700,1716,1732,1749,1766,1782,1798,1815,1831,1847,1863,1880,1896,1912,1928,1945,1961,\
	1977,1993,2009,2025,2041,2057,2073,2089,2104,2121,2137,2153,2168,2184,2200,2216,2231,2248,2263,2278,2294,2310,2326,2341,2357,2373,2388,2403,2419,2434,2450,2466,2481,2496,2512,2527,2543,2558,2573,2589,2604,2619,2635,2650,2665,2680,2696,2711,2726,2741,2757,2771,2787,2801,2817,2832,2847,2862,2877,2892,2907,2922,2937,2952,2967,2982,2997,3012,3027,3041,3057,3071,3086,3101,3116,3130,3145,3160,3175,3190,3204,3219,3234,3248,3263,3278,3293,3307,3322,3337,3351,3365,3380,3394,3409,3424,3438,3453,\
	3468,3482,3497,3511,3525,3540,3554,3569,3584,3598,3612,3626,3641,3655,3670,3684,3699,3713,3727,3742,3756,3770,3784,3799,3813,3827,3841,3856,3870,3884,3898,3912,3927,3941,3955,3969,3983,3997,4011,4026,4039,4054,4068,4082,4095
};

const HI_U16 OV2710_LINE_GAMMA_16A[257]=
{
	0,68,138,209,281,353,426,498,570,640,710,777,843,907,967,1025,1079,1130,1178,1224,1267,1308,1348,1385,1421,1456,1489,1521,1553,1584,1615,1645,1676,1706,1735,1762,1789,1815,1839,1863,1887,1910,1932,1954,1975,1996,2018,2039,2060,2081,2102,2122,2141,2161,2180,2198,2217,2235,2252,2270,2288,2305,2322,2339,2356,2373,2390,2406,2422,2438,2454,2470,2486,2501,2516,2531,2546,2560,2575,2589,2603,2617,2630,2644,2657,2670,2682,2695,2707,2719,2731,2743,2755,2767,2779,2790,2802,2814,2825,2836,2848,2859,2870,2880,\
	2891,2902,2913,2924,2934,2945,2956,2967,2978,2989,3000,3011,3023,3034,3045,3057,3068,3079,3090,3102,3113,3124,3135,3145,3156,3167,3177,3188,3198,3208,3218,3229,3239,3249,3259,3269,3279,3288,3298,3308,3317,3326,3336,3345,3354,3363,3372,3381,3389,3398,3407,3415,3424,3432,3441,3449,3458,3466,3475,3483,3492,3500,3508,3516,3524,3533,3541,3549,3557,3565,3573,3582,3590,3598,3607,3615,3624,3632,3641,3650,3658,3667,3675,3684,3692,3700,3708,3716,3724,3732,3739,3747,3754,3761,3769,3776,3783,3790,3797,3804,\
	3811,3818,3824,3831,3838,3845,3852,3858,3865,3872,3878,3885,3892,3898,3905,3911,3917,3923,3929,3935,3941,3947,3952,3957,3963,3968,3973,3977,3982,3987,3992,3996,4001,4006,4010,4015,4020,4025,4030,4034,4039,4044,4048,4053,4058,4062,4067,4072,4076,4081,4086,4090,4095
};

const HI_U16 OV2710_LINE_GAMMA_H2_16A[257] =
{
	0,45,90,135,181,226,272,318,363,408,453,498,542,585,628,670,711,751,791,830,868,906,944,981,1017,1054,1090,1125,1160,1196,1231,1265,1300,1335,1369,1404,1438,1472,1506,1540,1573,1606,1639,1670,1702,1732,1762,1792,1820,1848,1874,1900,1926,1951,1975,1998,2022,2044,2067,2089,2111,2132,2153,2175,2196,2217,2238,2258,2278,2298,2317,2337,2356,2374,2393,2411,2429,2447,2464,2482,2499,2516,2533,2549,2565,2581,2596,2612,2627,2642,2657,2672,2687,2701,2716,2731,2746,2761,2776,2791,2806,2821,2836,2851,2865,2880,\
	2894,2909,2923,2937,2951,2965,2978,2991,3004,3017,3030,3042,3055,3067,3079,3091,3103,3114,3126,3138,3149,3161,3172,3183,3194,3206,3216,3227,3238,3249,3259,3270,3280,3291,3301,3311,3321,3331,3341,3351,3361,3370,3380,3390,3399,3408,3418,3427,3436,3445,3454,3463,3472,3481,3490,3499,3507,3516,3525,3533,3542,3550,3558,3567,3575,3583,3591,3599,3607,3614,3622,3629,3637,3644,3651,3658,3665,3672,3679,3685,3692,3699,3705,3712,3719,3725,3732,3739,3746,3752,3759,3766,3773,3779,3786,3793,3799,3806,3812,\
	3819,3825,3832,3838,3844,3850,3856,3863,3869,3875,3881,3887,3893,3899,3905,3910,3916,3922,3928,3933,3939,3945,3950,3956,3962,3967,3973,3978,3983,3989,3994,3999,4005,4010,4015,4020,4026,4031,4036,4041,4046,4051,4056,4061,4066,4071,4076,4081,4085,4090,4095,4095
};

const HI_U16 OV2710_LINE_GAMMA_H_16A[257] =
{
	0,57,116,175,234,294,354,413,473,531,589,647,703,757,810,862,911,959,1005,1051,1095,1138,1180,1221,1261,1300,1339,1376,1412,1447,1482,1515,1548,1580,1610,1639,1667,1693,1719,1744,1768,1792,1815,1837,1860,1882,1904,1926,1948,1970,1991,2012,2033,2053,2073,2092,2112,2131,2149,2168,2186,2205,2223,2242,2260,2278,2297,2315,2333,2351,2368,2386,2404,2421,2438,2455,2472,2489,2506,2523,2539,2555,2572,2588,2604,2620,2636,2652,2667,2683,2698,2713,2728,2743,2758,2772,2786,2800,2813,2827,2840,2852,2865,2878,\
	2890,2902,2914,2926,2938,2950,2962,2974,2986,2998,3009,3021,3032,3043,3055,3066,3077,3089,3101,3113,3125,3137,3149,3161,3172,3183,3194,3206,3216,3227,3238,3249,3259,3270,3280,3291,3301,3311,3321,3331,3341,3351,3361,3370,3380,3390,3399,3408,3418,3427,3436,3445,3454,3463,3472,3481,3490,3499,3507,3516,3525,3533,3542,3550,3558,3567,3575,3583,3591,3599,3607,3614,3622,3629,3637,3644,3651,3658,3665,3672,3679,3685,3692,3699,3705,3712,3719,3725,3732,3739,3746,3752,3759,3766,3773,3779,3786,3793,3799,\
	3806,3812,3819,3825,3832,3838,3844,3850,3856,3863,3869,3875,3881,3887,3893,3899,3905,3910,3916,3922,3928,3933,3939,3945,3950,3956,3962,3967,3973,3978,3983,3989,3994,3999,4005,4010,4015,4020,4026,4031,4036,4041,4046,4051,4056,4061,4066,4071,4076,4081,4085,4090,4095,4095
};

const HI_U16 IMX123_WDR_GAMMA[257] =
{
	0,1,2,4,8,12,17,23,30,38,47,57,68,79,92,105,120,133,147,161,176,192,209,226,243,260,278,296,317,340,365,390,416,440,466,491,517,538,561,584,607,631,656,680,705,730,756,784,812,835,858,882,908,934,958,982,1008,1036,1064,1092,1119,1143,1167,1192,1218,1243,1269,1296,1323,1351,1379,1408,1434,1457,1481,1507,1531,1554,1579,1603,1628,1656,1683,1708,1732,1756,1780,1804,1829,1854,1877,1901,1926,1952,1979,2003,2024,2042,2062,2084,2106,2128,2147,2168,2191,2214,2233,2256,2278,2296,2314,2335,2352,2373,2391,2412,2431,2451,2472,2492,2513,2531,\
	2547,2566,2581,2601,2616,2632,2652,2668,2688,2705,2721,2742,2759,2779,2796,2812,2826,2842,2857,2872,2888,2903,2920,2934,2951,2967,2983,3000,3015,3033,3048,3065,3080,3091,3105,3118,3130,3145,3156,3171,3184,3197,3213,3224,3240,3252,3267,3281,3295,3310,3323,3335,3347,3361,3372,3383,3397,3409,3421,3432,3447,3459,3470,3482,3497,3509,3521,3534,3548,3560,3572,3580,3592,3602,3613,3625,3633,3646,3657,3667,3679,3688,3701,3709,3719,3727,3736,3745,3754,3764,3773,3781,3791,3798,3806,3816,3823,3833,3840,3847,3858,3865,3872,3879,3888,3897,\
	3904,3911,3919,3926,3933,3940,3948,3955,3962,3970,3973,3981,3988,3996,4003,4011,4018,4026,4032,4037,4045,4053,4057,4064,4072,4076,4084,4088,4095
};
/*
const HI_U16 OV4689_LINE_GAMMA[257] = 
{
	
	0x0,0x43,0x87,0xCC,0x112,0x158,0x19E,0x1E4,0x22A,0x26F,0x2B3,0x2F5,0x335,0x374,0x3B0,0x3E9,0x41F,0x452,0x483,0x4B2,0x4DF,0x50A,0x533,0x55B,0x581,0x5A6,0x5CA,0x5ED,0x60F,0x631,0x652,0x673,0x694,0x6B4,0x6D3,0x6F1,0x70E,0x72B,0x746,0x760,0x77A,0x794,0x7AC,0x7C5,0x7DD,0x7F5,0x80D,0x824,0x83C,0x854,0x86B,0x882,0x899,0x8AF,0x8C5,0x8DB,0x8F0,0x905,0x91A,0x92E,0x941,0x955,0x968,0x97A,0x98C,0x99D,0x9AE,0x9BE,0x9CE,0x9DD,0x9EB,0x9FA,0xA08,0xA15,0xA23,0xA30,0xA3D,0xA4B,0xA58,0xA65,0xA73,0xA81,0xA8E,0xA9C,0xAA9,0xAB6,0xAC3,0xAD0,0xADD,0xAEA,\
	0xAF7,0xB04,0xB10,0xB1D,0xB29,0xB36,0xB42,0xB4E,0xB5A,0xB66,0xB72,0xB7E,0xB89,0xB95,0xBA0,0xBAB,0xBB7,0xBC2,0xBCD,0xBD9,0xBE4,0xBEF,0xBFA,0xC05,0xC10,0xC1C,0xC27,0xC32,0xC3D,0xC49,0xC54,0xC5F,0xC69,0xC74,0xC7F,0xC89,0xC93,0xC9D,0xCA7,0xCB0,0xCBA,0xCC3,0xCCB,0xCD4,0xCDD,0xCE5,0xCED,0xCF5,0xCFD,0xD05,0xD0D,0xD15,0xD1D,0xD25,0xD2D,0xD35,0xD3D,0xD45,0xD4D,0xD55,0xD5D,0xD65,0xD6D,0xD75,0xD7D,0xD84,0xD8C,0xD94,0xD9B,0xDA3,0xDAA,0xDB1,0xDB8,0xDBF,0xDC6,0xDCD,0xDD4,0xDDA,0xDE1,0xDE8,0xDEE,0xDF5,0xDFB,0xE02,0xE09,0xE0F,0xE16,0xE1D,0xE24,\
	0xE2A,0xE31,0xE38,0xE3F,0xE46,0xE4D,0xE53,0xE5A,0xE61,0xE68,0xE6F,0xE76,0xE7D,0xE84,0xE8B,0xE92,0xE99,0xEA1,0xEA8,0xEAF,0xEB7,0xEBE,0xEC5,0xECC,0xED4,0xEDB,0xEE2,0xEE9,0xEEF,0xEF6,0xEFC,0xF03,0xF09,0xF0F,0xF15,0xF1B,0xF21,0xF27,0xF2D,0xF33,0xF39,0xF3E,0xF44,0xF4A,0xF4F,0xF55,0xF5B,0xF60,0xF66,0xF6B,0xF71,0xF76,0xF7C,0xF81,0xF87,0xF8C,0xF91,0xF97,0xF9C,0xFA1,0xFA7,0xFAC,0xFB1,0xFB7,0xFBC,0xFC1,0xFC6,0xFCB,0xFD1,0xFD6,0xFDB,0xFE0,0xFE5,0xFEA,0xFEF,0xFF5,0xFFA,0xFFF

};*/


const HI_U16 OV4689_LINE_GAMMA[257]=
{

	0x0,0x50,0xA3,0xF5,0x148,0x199,0x1E8,0x233,0x27A,0x2BD,0x2FC,0x338,0x372,0x3AA,0x3DF,0x414,0x447,0x479,0x4A9,0x4D7,0x504,0x52F,0x559,0x582,0x5A9,0x5CF,0x5F3,0x615,0x636,0x656,0x676,0x695,0x6B4,0x6D3,0x6F1,0x70F,0x72C,0x748,0x764,0x77F,0x79A,0x7B4,0x7CE,0x7E7,0x7FF,0x817,0x82E,0x845,0x85C,0x872,0x887,0x89C,0x8B1,0x8C5,0x8D9,0x8EC,0x900,0x913,0x927,0x93A,0x94D,0x95F,0x971,0x983,0x994,0x9A5,0x9B5,0x9C5,0x9D5,0x9E4,0x9F3,0xA01,0xA10,0xA1E,0xA2C,0xA3A,0xA47,0xA54,0xA61,0xA6E,0xA7B,0xA88,0xA94,0xAA0,0xAAC,0xAB8,\
	0xAC5,0xAD1,0xADD,0xAEA,0xAF6,0xB03,0xB10,0xB1D,0xB29,0xB36,0xB42,0xB4E,0xB5A,0xB66,0xB72,0xB7E,0xB89,0xB95,0xBA0,0xBAB,0xBB7,0xBC2,0xBCD,0xBD9,0xBE4,0xBEF,0xBFA,0xC05,0xC10,0xC1C,0xC27,0xC32,0xC3D,0xC49,0xC54,0xC5F,0xC69,0xC74,0xC7F,0xC89,0xC93,0xC9D,0xCA7,0xCB0,0xCBA,0xCC3,0xCCB,0xCD4,0xCDD,0xCE5,0xCED,0xCF5,0xCFD,0xD05,0xD0D,0xD15,0xD1D,0xD25,0xD2D,0xD35,0xD3D,0xD45,0xD4D,0xD55,0xD5D,0xD65,0xD6D,0xD75,0xD7D,0xD84,0xD8C,0xD94,0xD9B,0xDA3,0xDAA,0xDB1,0xDB8,0xDBF,0xDC6,0xDCD,0xDD4,0xDDA,0xDE1,0xDE8,\
	0xDEE,0xDF5,0xDFB,0xE02,0xE09,0xE0F,0xE16,0xE1D,0xE24,0xE2A,0xE31,0xE38,0xE3F,0xE46,0xE4D,0xE53,0xE5A,0xE61,0xE68,0xE6F,0xE76,0xE7D,0xE84,0xE8B,0xE92,0xE99,0xEA1,0xEA8,0xEAF,0xEB7,0xEBE,0xEC5,0xECC,0xED4,0xEDB,0xEE2,0xEE9,0xEEF,0xEF6,0xEFC,0xF03,0xF09,0xF0F,0xF15,0xF1B,0xF21,0xF27,0xF2D,0xF33,0xF39,0xF3E,0xF44,0xF4A,0xF4F,0xF55,0xF5B,0xF60,0xF66,0xF6B,0xF71,0xF76,0xF7C,0xF81,0xF87,0xF8C,0xF91,0xF97,0xF9C,0xFA1,0xFA7,0xFAC,0xFB1,0xFB7,0xFBC,0xFC1,0xFC6,0xFCB,0xFD1,0xFD6,0xFDB,0xFE0,0xFE5,0xFEA,0xFEF,0xFF5,0xFFA,0xFFF
};

const HI_U16 OV4689_NIGHT_GAMMA[257] =
{
	0x0,0x35,0x6B,0xA1,0xD7,0x10E,0x145,0x17C,0x1B2,0x1E8,0x21D,0x252,0x286,0x2B8,0x2EA,0x31A,0x348,0x375,0x3A1,0x3CB,0x3F5,0x41D,0x445,0x46C,0x492,0x4B7,0x4DC,0x500,0x524,0x547,0x56A,0x58D,0x5AF,0x5D1,0x5F3,0x614,0x634,0x654,0x673,0x692,0x6B1,0x6CE,0x6EC,0x709,0x726,0x742,0x75E,0x77A,0x795,0x7B0,0x7CA,0x7E4,0x7FE,0x817,0x830,0x849,0x861,0x878,0x890,0x8A6,0x8BD,0x8D3,0x8E9,0x8FF,0x914,0x929,0x93D,0x951,0x964,0x977,0x98A,0x99C,0x9AE,0x9C0,0x9D2,0x9E3,0x9F4,0xA05,0xA16,0xA27,0xA38,0xA49,0xA59,0xA6A,0xA7A,0xA8B,0xA9B,0xAAB,0xABA,0xACA,0xAD9,\
	0xAE8,0xAF7,0xB06,0xB14,0xB22,0xB30,0xB3E,0xB4B,0xB58,0xB64,0xB71,0xB7D,0xB89,0xB95,0xBA0,0xBAC,0xBB7,0xBC2,0xBCE,0xBD9,0xBE5,0xBF0,0xBFB,0xC07,0xC12,0xC1E,0xC29,0xC34,0xC40,0xC4B,0xC56,0xC61,0xC6C,0xC76,0xC81,0xC8C,0xC96,0xCA0,0xCAA,0xCB4,0xCBE,0xCC7,0xCD1,0xCDA,0xCE3,0xCEC,0xCF5,0xCFE,0xD07,0xD10,0xD18,0xD21,0xD2A,0xD32,0xD3A,0xD43,0xD4B,0xD53,0xD5A,0xD62,0xD6A,0xD71,0xD79,0xD80,0xD88,0xD90,0xD97,0xD9F,0xDA6,0xDAE,0xDB6,0xDBE,0xDC5,0xDCD,0xDD5,0xDDD,0xDE5,0xDED,0xDF5,0xDFD,0xE04,0xE0C,0xE14,0xE1B,0xE23,0xE2A,0xE31,0xE39,0xE40,0xE47,0xE4E,\
	0xE55,0xE5C,0xE63,0xE6A,0xE71,0xE77,0xE7E,0xE85,0xE8B,0xE92,0xE98,0xE9E,0xEA5,0xEAB,0xEB1,0xEB7,0xEBC,0xEC2,0xEC8,0xECE,0xED3,0xED9,0xEDF,0xEE5,0xEEA,0xEF0,0xEF6,0xEFC,0xF02,0xF08,0xF0E,0xF14,0xF1A,0xF20,0xF26,0xF2B,0xF31,0xF37,0xF3D,0xF43,0xF49,0xF4E,0xF54,0xF5A,0xF5F,0xF65,0xF6A,0xF6F,0xF75,0xF7A,0xF80,0xF85,0xF8A,0xF8F,0xF95,0xF9A,0xF9F,0xFA5,0xFAA,0xFAF,0xFB5,0xFBA,0xFBF,0xFC5,0xFCA,0xFCF,0xFD5,0xFDA,0xFDF,0xFE4,0xFEA,0xFEF,0xFF4,0xFFA,0xFFF
};



HI_U8 gu8SensorMode;
static int sensorid;
int sensorType=0;
//static int user_select_luma=80;
static int user_sensor_mod=0;
static int user_select_gamma=0;

static signed int user_set_contrast=128;
static BOOL nightflag = HI_FALSE;
static signed int user_set_luma=128;

static BOOL  bUserWDROpen =FALSE; //只用来夜视白天的宽动态切换标志

static float CURRENT_SENSOR_FPS =25.0;
static int   USER_SENSOR_FPS =25;
static int   LowFpsRatio =16;
static pthread_mutex_t isp_rf_mutex = PTHREAD_MUTEX_INITIALIZER;

#define OV4689_LINEAR_DAY_LUM    68
#define OV4689_LINEAR_NIGHT_LUM  80

#define OV4689_WDR_DAY_LUM       64
#define OV4689_WDR_NIGHT_LUM     64



#define AR0230_LINEAR_DAY_LUM    64
#define AR0230_LINEAR_NIGHT_LUM  78

#define AR0230_WDR_DAY_LUM       93
#define AR0230_WDR_NIGHT_LUM     93


#define IMX290_DAY_LUM    68
#define IMX290_NIGHT_LUM  80

#define IMX123_DAY_LUM    72
#define IMX123_NIGHT_LUM  80

#define IMX178_DAY_LUM    68
#define IMX178_NIGHT_LUM  78

#define AR0237_LINEAR_DAY_LUM    70
#define AR0237_LINEAR_NIGHT_LUM  78

#define AR0237_WDR_DAY_LUM       78
#define AR0237_WDR_NIGHT_LUM     78

#define OV2710_LINEAR_DAY_LUM    68
#define OV2710_LINEAR_NIGHT_LUM  80
#define CP_LINEAR_LUM            93


#define IMX185_LINEAR_DAY_LUM    72
#define IMX185_LINEAR_NIGHT_LUM  78

int jv_sensor_wdr_switch(BOOL bWDREn);
int jv_sensor_get_wdr_mode();

/*
BOOL __check_wdr() //判断是否3M wdr 版本
{

	char *devName;
	devName = hwinfo.devName;
	if (devName && (strstr(devName, "3MWDR")))
	{
		return TRUE;
	}
	else
		return FALSE;
}
BOOL __check_live() //判断是否中维直播模式
{

	char *devName;
	devName = hwinfo.devName;
	if (devName && (strstr(devName, "LIVE")))
	{
		return TRUE;
	}
	else
		return FALSE;
}*/

/*
static ISP_AE_ROUTE_S gstAERouteAttr_ov4689_20fps = 
{    
	7,    

	{        
		{56,  1024,   1},        
		{500, 1024,   1},       
		{500, 3072,   1},        
		{5500, 3072,   1},        
		{5500, 4800, 1},  
		{24290, 4800,   1},
		{24290, 20480,   1}
  	}
};*/
/*
static HI_U32 au32RouteNode[7][3]=		
{
	{56,  1024,   1},		 
	{900, 1024,   1},		
	{900, 3328,   1},		 
	{4500, 3328,   1},		  
	{4500, 5280, 1},  
	{24290, 5280,	1},
	{24290, 10240,	 1}
};*/


static HI_U32 AR0230_RouteNode[5][3]=		
{
	{10,  1024,   1},		 
	{15000, 1024,   1},		
	{15000, 4096,   1},		 
	{333333, 4096,   1},		  
	{333333, 49152, 1}
};


BOOL __check_cp() //判断是否为车牌特定版本。
{

	char *product;
	product = hwinfo.product;
	if (product && (strstr(product, "cp")||strstr(product, "CP")))
	{
		return TRUE;
	}
	else
		return FALSE;
}


int isp_wdr_init(BOOL bWdr)
{
	ISP_DEV IspDev =0;
	
	if(bWdr==FALSE)//线性
	{
		if(sensorType == SENSOR_OV4689)
		{
			if(strstr(hwinfo.product,"STC"))
			{
				HI_U8 SharpenAltD[]  = {0x30,0x30,0x30,0x30,0x30,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x12,0x10,0x10,0x10};
				HI_U8 SharpenAltUd[] =   {0x50,0x50,0x50,0x50,0x48,0x40,0x38,0x28,0x20,0x20,0x18,0x12,0x10,0x10,0x10,0x10};
				HI_U8 au8SharpenRGB[]={0x78,0x76,0x74,0x70,0x6a,0x60,0x58,0x50,0x40,0x30,0x20,0x16,0x12,0x12,0x12,0x12};
				
				
				ISP_SHARPEN_ATTR_S  pstSharpenAttr;
				HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
				memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
				memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
				memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
				HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.stSysGainRange.u32Max = 23*1024; 
				stExpAttr.u8AERunInterval=2;
				stExpAttr.stAuto.u8Tolerance =6;
				stExpAttr.stAuto.u8Speed = 40;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}
			else
			{
				//HI_U8 SharpenAltD[]  = {68,58,56,52,35,20,20,20,20,20,20,20,18,16,16,16};
				//HI_U8 SharpenAltUd[] = {106,96,88,80,52,30,30,30,30,30,24,18,16,16,16,16};
				//HI_U8 au8SharpenRGB[]={0x88,0x86,0x84,0x78,60,40,30,30,30,30,30,30,30,0x12,0x12,0x12};
				
				//ISP_SHARPEN_ATTR_S  pstSharpenAttr;
				//HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
				//memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
				//memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
				//memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
				//HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.stSysGainRange.u32Max = 64*1024; 
				stExpAttr.u8AERunInterval=2;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}
			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);
			
		}
		else if(sensorType == SENSOR_AR0230)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			stExpAttr.stAuto.stSysGainRange.u32Max = 75*1024; 
			stExpAttr.u8AERunInterval=2;
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


			ISP_CR_ATTR_S  pstCRAttr;
			HI_MPI_ISP_GetCrosstalkAttr(IspDev,&pstCRAttr);
			pstCRAttr.u8Sensitivity =20;
			pstCRAttr.u16Slope =100;
			int i;
			for(i=0;i<8;i++)
			{
				pstCRAttr.au8Strength[i]= 15;
			}
			HI_MPI_ISP_SetCrosstalkAttr(IspDev,&pstCRAttr);
			
			ISP_AE_ROUTE_S  pstAERouteAttr;
			HI_MPI_ISP_GetAERouteAttr(IspDev,&pstAERouteAttr);		
			pstAERouteAttr.u32TotalNum=0;
			HI_MPI_ISP_SetAERouteAttr(IspDev,&pstAERouteAttr);



		}
		else if(sensorType == SENSOR_IMX290)
		{
			ISP_NR_ATTR_S  pstNRAttr;
			HI_MPI_ISP_GetNRAttr(0,&pstNRAttr);
			//HI_U8 au8NRtbl[]={0x04,0x0a,0x0f,0x12,0x16,0x20,0x35,0x41,0x46,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c};	
			HI_U8 au8NRtbl[]= {0x04,0x0a,0x0f,0x12,0x16,0x20,0x35,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c};
			memcpy(pstNRAttr.stAuto.au8Thresh,au8NRtbl,16);
			HI_MPI_ISP_SetNRAttr(0,&pstNRAttr);
		}
		else if(sensorType == SENSOR_AR0237||sensorType == SENSOR_AR0237DC)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			stExpAttr.stAuto.stSysGainRange.u32Max = 32*1024; 
			stExpAttr.u8AERunInterval=2;
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			
			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_FALSE;
			pstDciParam.u32ContrastGain = 10;
			pstDciParam.u32LightGain = 10;
			pstDciParam.u32BlackGain = 10;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);
			ISP_CR_ATTR_S  pstCRAttr;
			HI_MPI_ISP_GetCrosstalkAttr(IspDev,&pstCRAttr);
			pstCRAttr.u8Sensitivity =20;
			pstCRAttr.u16Slope =100;
			int i;
			for(i=0;i<8;i++)
			{
				pstCRAttr.au8Strength[i]= 15;
			}
			HI_MPI_ISP_SetCrosstalkAttr(IspDev,&pstCRAttr);

			ISP_SHADING_ATTR_S pstShadingAttr;
			HI_MPI_ISP_GetShadingAttr(0,&pstShadingAttr);
			pstShadingAttr.bEnable =HI_TRUE;
			HI_MPI_ISP_SetShadingAttr(0,&pstShadingAttr);

			//ISP_AWB_ATTR_EX_S pstAWBAttrEx;
			//HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
			//pstAWBAttrEx.stInOrOut.enOpType = OP_TYPE_MANUAL;
			//pstAWBAttrEx.stInOrOut.bOutdoorStatus  = HI_TRUE;
			//HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);
			ISP_AWB_ATTR_EX_S pstAWBAttrEx;
			HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
			pstAWBAttrEx.stInOrOut.enOpType = OP_TYPE_AUTO;
			pstAWBAttrEx.stInOrOut.u32OutThresh = 33000;
			HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);
		}
		else if(sensorType == SENSOR_IMX123)
		{
			
			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_FALSE;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);
			
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			//stExpAttr.stAuto.stSysGainRange.u32Max = 280*1024; 
			stExpAttr.stAuto.stSysGainRange.u32Max = 512*1024; 
			stExpAttr.u8AERunInterval=2;
			//stExpAttr.stAuto.u16EVBias = 1024;
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


			ISP_AE_ROUTE_S  pstAERouteAttr;
			HI_MPI_ISP_GetAERouteAttr(IspDev,&pstAERouteAttr);		
			pstAERouteAttr.u32TotalNum=0;
			HI_MPI_ISP_SetAERouteAttr(IspDev,&pstAERouteAttr);
		}
		else if(sensorType == SENSOR_IMX185)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			stExpAttr.stAuto.stSysGainRange.u32Max = 150*1024; 
			stExpAttr.u8AERunInterval=2;
			if(__check_cp())
			{
				stExpAttr.stAuto.stSysGainRange.u32Max = 45*1024;
				stExpAttr.stAuto.stExpTimeRange.u32Max = 7500;			
			}

			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_FALSE;
			pstDciParam.u32ContrastGain = 20;
			pstDciParam.u32LightGain = 20;
			pstDciParam.u32BlackGain = 20;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);
			
		}
		return 0;
	}
	else   //wdr 模式
	{
		

		if(sensorType == SENSOR_OV4689)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			stExpAttr.stAuto.stSysGainRange.u32Max = 19*1024; 
			stExpAttr.stAuto.u16EVBias = 1024;
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

			ISP_WDR_FS_ATTR_S  pstFSWDRAttr;
			HI_MPI_ISP_GetFSWDRAttr(IspDev,&pstFSWDRAttr);
			pstFSWDRAttr.bMotionComp = HI_FALSE; //关闭运动补偿
			HI_MPI_ISP_SetFSWDRAttr(IspDev,&pstFSWDRAttr);


			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_TRUE;
			pstDciParam.u32ContrastGain = 10;
			pstDciParam.u32LightGain = 10;
			pstDciParam.u32BlackGain = 10;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);

			ISP_WDR_EXPOSURE_ATTR_S pstWDRExpAttr;
			HI_MPI_ISP_GetWDRExposureAttr(IspDev,&pstWDRExpAttr);
			pstWDRExpAttr.enExpRatioType  = OP_TYPE_AUTO;
			pstWDRExpAttr.u32ExpRatioMax =480;
			pstWDRExpAttr.u32ExpRatioMin = 180;
			HI_MPI_ISP_SetWDRExposureAttr(IspDev,&pstWDRExpAttr); 

		}
		else if(sensorType == SENSOR_AR0230)
		{
			ISP_WDR_FS_ATTR_S  pstFSWDRAttr;
			HI_MPI_ISP_GetFSWDRAttr(IspDev,&pstFSWDRAttr);
			pstFSWDRAttr.bMotionComp = HI_FALSE; //关闭运动补偿
			HI_MPI_ISP_SetFSWDRAttr(IspDev,&pstFSWDRAttr);

			ISP_AE_ROUTE_S  pstAERouteAttr;
			HI_MPI_ISP_GetAERouteAttr(IspDev,&pstAERouteAttr);
				
			pstAERouteAttr.u32TotalNum=5;
			memcpy(pstAERouteAttr.astRouteNode,AR0230_RouteNode,sizeof(AR0230_RouteNode));
			HI_MPI_ISP_SetAERouteAttr(IspDev,&pstAERouteAttr);

			ISP_DRC_ATTR_S pstDRCAttr;
            HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);

			pstDRCAttr.enOpType= OP_TYPE_MANUAL;
			pstDRCAttr.stManual.u32Strength=220;
            HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);    

			if(__check_cp())
			{
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.stSysGainRange.u32Max = 32*1024; 
				stExpAttr.stAuto.stExpTimeRange.u32Max =13324;
				stExpAttr.stAuto.u8Compensation  = 48;
				HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			}
			
			ISP_CR_ATTR_S  pstCRAttr;
			HI_MPI_ISP_GetCrosstalkAttr(IspDev,&pstCRAttr);
			pstCRAttr.u8Sensitivity =20;
			pstCRAttr.u16Slope =100;
			int i;
			for(i=0;i<8;i++)
			{
				pstCRAttr.au8Strength[i]= 15;
			}
			HI_MPI_ISP_SetCrosstalkAttr(IspDev,&pstCRAttr);



		}
		else if(sensorType == SENSOR_AR0237||sensorType == SENSOR_AR0237DC)
		{
			ISP_EXPOSURE_ATTR_S stExpAttr;
			HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
			stExpAttr.stAuto.stSysGainRange.u32Max = 32*1024; 
			HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
			
			ISP_DRC_ATTR_S pstDRCAttr;
            HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
			pstDRCAttr.enOpType= OP_TYPE_AUTO;
			pstDRCAttr.stAuto.u32Strength =128;
            HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr); 
			
			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_TRUE;
			pstDciParam.u32ContrastGain = 10;
			pstDciParam.u32LightGain = 10;
			pstDciParam.u32BlackGain = 10;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);

			ISP_WDR_EXPOSURE_ATTR_S pstWDRExpAttr;
			HI_MPI_ISP_GetWDRExposureAttr(IspDev,&pstWDRExpAttr);
			pstWDRExpAttr.enExpRatioType  = OP_TYPE_AUTO;
			//pstWDRExpAttr.u32ExpRatio = 190;
			pstWDRExpAttr.u32ExpRatioMax =360;
			pstWDRExpAttr.u32ExpRatioMin = 160;

			//pstWDRExpAttr.enExpRatioType  = OP_TYPE_MANUAL;
			//pstWDRExpAttr.u32ExpRatio = 190;
			HI_MPI_ISP_SetWDRExposureAttr(IspDev,&pstWDRExpAttr); 

			ISP_CR_ATTR_S  pstCRAttr;
			HI_MPI_ISP_GetCrosstalkAttr(IspDev,&pstCRAttr);
			pstCRAttr.u8Sensitivity =20;
			pstCRAttr.u16Slope =100;
			int i;
			for(i=0;i<8;i++)
			{
				pstCRAttr.au8Strength[i]= 15;
			}
			HI_MPI_ISP_SetCrosstalkAttr(IspDev,&pstCRAttr);

			ISP_SHADING_ATTR_S pstShadingAttr;
			HI_MPI_ISP_GetShadingAttr(0,&pstShadingAttr);
			pstShadingAttr.bEnable =HI_FALSE;
			HI_MPI_ISP_SetShadingAttr(0,&pstShadingAttr);

			ISP_AWB_ATTR_EX_S pstAWBAttrEx;
			HI_MPI_ISP_GetAWBAttrEx(0, &pstAWBAttrEx);
			pstAWBAttrEx.stInOrOut.enOpType = OP_TYPE_AUTO;
			pstAWBAttrEx.stInOrOut.u32OutThresh = 9000;
			HI_MPI_ISP_SetAWBAttrEx(0, &pstAWBAttrEx);	
		}
		else if(sensorType == SENSOR_IMX123)
		{
			ISP_WDR_EXPOSURE_ATTR_S pstWDRExpAttr;
			HI_MPI_ISP_GetWDRExposureAttr(IspDev,&pstWDRExpAttr);
			pstWDRExpAttr.enExpRatioType  = OP_TYPE_AUTO;
			//pstWDRExpAttr.u32ExpRatio = 190;
			pstWDRExpAttr.u32ExpRatioMax =580;
			pstWDRExpAttr.u32ExpRatioMin = 180;
			HI_MPI_ISP_SetWDRExposureAttr(IspDev,&pstWDRExpAttr);
			
			VI_DCI_PARAM_S  pstDciParam;
			HI_MPI_VI_GetDCIParam(0,&pstDciParam);
			pstDciParam.bEnable =HI_TRUE;
			pstDciParam.u32ContrastGain = 20;
			pstDciParam.u32LightGain = 20;
			pstDciParam.u32BlackGain = 10;
			HI_MPI_VI_SetDCIParam(0,&pstDciParam);

			ISP_AE_ROUTE_S  pstAERouteAttr;
			HI_MPI_ISP_GetAERouteAttr(IspDev,&pstAERouteAttr);		
			pstAERouteAttr.u32TotalNum=0;
			HI_MPI_ISP_SetAERouteAttr(IspDev,&pstAERouteAttr);


		}
		


	}
	return 0;
}



//向物理寄存器设置data
int set_hwaddr_value(HI_U32 hwaddr,HI_U32 newdata,HI_U32 *old_data)
{
	 VOID* pMem  = NULL;
	 pMem = memmap(hwaddr, DEFAULT_MD_LEN);
	 if(old_data)
	 *old_data = *(U32*)pMem;                               
	 *(U32*)pMem = newdata;
	 return 0;
}

int reg_spi_read(unsigned int addr)
{
	int fd = -1;
	unsigned int value = 0;
	int ret=0;
	
	// spi硬件使能
	set_hwaddr_value(0x200f0050,0x1,NULL);
	set_hwaddr_value(0x200f0054,0x1,NULL);
	set_hwaddr_value(0x200f0058,0x1,NULL);
	set_hwaddr_value(0x200f005c,0x1,NULL);

	usleep(100*1000);
	
	fd = open("/dev/spidev0.0", 0);
	if (fd < 0) 
	{		
		printf("Open spidev0.0 error!\n");	
		return -1;
	}
	value = SPI_MODE_3 | SPI_LSB_FIRST;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &value);
	if (ret < 0)    
	{        
		printf("ioctl SPI_IOC_WR_MODE err, value = %d ret = %d\n", value, ret);        
		return ret;    
	}
	value = 8;    
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &value);    
	if (ret < 0)    
	{        
		printf("ioctl SPI_IOC_WR_BITS_PER_WORD err, value = %d ret = %d\n",value, ret);        
		return ret;    
	}    
	value = 2000000;    
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &value);    
	if (ret < 0)    
	{        
		printf("ioctl SPI_IOC_WR_MAX_SPEED_HZ err, value = %d ret = %d\n",value, ret);       
		return ret;    
	}

	struct spi_ioc_transfer mesg[1];    
	unsigned char  tx_buf[8] = {0};    
	unsigned char  rx_buf[8] = {0};        
	tx_buf[0] = (addr & 0xff00) >> 8;    
	tx_buf[0] |= 0x80;    
	tx_buf[1] = addr & 0xff;    
	tx_buf[2] = 0;    
	memset(mesg, 0, sizeof(mesg));    
	mesg[0].tx_buf = (__u32)tx_buf;    
	mesg[0].len    = 3;    
	mesg[0].rx_buf = (__u32)rx_buf;    
	mesg[0].cs_change = 1;    
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), mesg);    
	if (ret  < 0)
	{          
		printf("SPI_IOC_MESSAGE error \n");          
		return -1;      
	}    
	return rx_buf[2];

}



int reg_i2c_read(unsigned int device_addr,unsigned int reg_addr,unsigned int reg_width,unsigned int data_width)
{
	int fd = -1;
	int ret;
	unsigned int i2c_num, reg_addr_end;
	unsigned int data;
	char recvbuf[4];
	
	//I2c硬件使能
    set_hwaddr_value(0x200f0050,0x2,NULL);
	set_hwaddr_value(0x200f0054,0x2,NULL);
	set_hwaddr_value(0x200f0058,0x0,NULL);
	set_hwaddr_value(0x200f005c,0x0,NULL);

	usleep(100*1000);

	memset(recvbuf, 0x0, 4);
	
	i2c_num =0;
	
	if (i2c_num == 0)
		fd = open("/dev/i2c-0", O_RDWR);
	else if (i2c_num == 1)
		fd = open("/dev/i2c-1", O_RDWR);
	else if (i2c_num == 2)
		fd = open("/dev/i2c-2", O_RDWR);
	else
		return -1;

	if (fd<0)
	{
		printf("Open i2c dev error!\n");
		return -1;
	}
	
	reg_addr_end = reg_addr;
	ret = ioctl(fd, I2C_SLAVE_FORCE, device_addr);
	
	if (reg_width == 2)
		ret = ioctl(fd, I2C_16BIT_REG, 1);
	else
		ret = ioctl(fd, I2C_16BIT_REG, 0);

	if (data_width == 2)
		ret = ioctl(fd, I2C_16BIT_DATA, 1);
	else
		ret = ioctl(fd, I2C_16BIT_DATA, 0);
	
	if (reg_width == 2) 
	{
		recvbuf[0] = reg_addr & 0xff;
		recvbuf[1] = (reg_addr >> 8) & 0xff;
	} 
	else
		recvbuf[0] = reg_addr & 0xff;

	ret = read(fd, recvbuf, reg_width);
	if (ret < 0)
	{
		printf("CMD_I2C_READ error!\n");
		close(fd);
		return -1;
	}

	if (data_width == 2)
	{
		data = recvbuf[0] | (recvbuf[1] << 8);
	} 
	else
		data = recvbuf[0];

	printf("0x%x 0x%x\n", reg_addr, data);
	
	close(fd);
	return data;
}




int isp_ioctl(int fd,int cmd,unsigned long value)
{
	ISP_DEV IspDev = 0;

	switch (cmd)
	{


#if 1
		case ADJ_SCENE://场景选择
		{

			if(value == IN_DOOR)
			{
				
			}
			else if(value == OUT_DOOR)
			{
				
			}
            else if(value == NIGHT_MODE)
            {


                ISP_SATURATION_ATTR_S pstSatAttr;         //设置黑白
                pstSatAttr.enOpType = OP_TYPE_MANUAL;
                pstSatAttr.stManual.u8Saturation = 0;
                HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);
                    
            }
            else if(value == DAY_MODE)
            {

                 
                ISP_SATURATION_ATTR_S pstSatAttr;
                pstSatAttr.enOpType = OP_TYPE_MANUAL;
                pstSatAttr.stManual.u8Saturation = 0x80;
                HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);//恢复彩色模式
                    
            }
            else if(CHECK_NIGHT== value)
            {
  				//entering night mode
  				nightflag = HI_TRUE;
				
				if(sensorid == SENSOR_OV4689)
				{
					jv_sensor_wdr_switch(FALSE);	 //夜间关掉WDR，提升低照度效?
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					//memcpy(&pstGammaAttr.u16Table,&u16Gamma_linear_h,257 * 2);
					memcpy(&pstGammaAttr.u16Table,&OV4689_NIGHT_GAMMA,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

					HI_U8 SharpenAltD[]  = {72,80,80,80,90,95,110,110,100,30,30,30,30,30,30,30};
					HI_U8 SharpenAltUd[] = {96,86,82,65,45,40,30,30,30,40,40,40,20,20,20,20};
					HI_U8 au8SharpenRGB[]={132,105,100,78,76,60,50,50,50,50,50,50,50,50,50,50};
				
				
					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

				}
				if(sensorid == SENSOR_AR0230)
				{
					jv_sensor_wdr_switch(FALSE);
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

					//HI_U8 SharpenAltD[]  =  {45,50,50,50,45,43,42,40,0x22,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
					HI_U8 SharpenAltD[]  = {45,50,55,75,110,110,110,80,0x22,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
					HI_U8 SharpenAltUd[] =  {120,120,90,80,65,55,45,35,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
					HI_U8 au8SharpenRGB[]=  {110,110,115,110,90,65,50,40,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
				
				
					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);


					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 50*1024; 
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

				}
				if(sensorid == SENSOR_IMX185)
				{
					//jv_sensor_wdr_switch(0);	 //夜间关掉WDR，提升低照度效?
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

				}
				if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
				{
					jv_sensor_wdr_switch(FALSE);
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

					HI_U8 SharpenAltD[]  = {62,62,60,80,80,100,85,55,0x22,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
					HI_U8 SharpenAltUd[] = {123,117,107,75,56,32,28,20,16,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
					HI_U8 au8SharpenRGB[]= {113,113,103,80,60,40,32,28,24,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
					
					

					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 28*1024; 
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

				}
				if(sensorid == SENSOR_OV2710)
				{
					//jv_sensor_wdr_switch(0);
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 30*1024; 
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


					HI_U8 SharpenAltD[]  =  {65,63,62,68,68,66,50,0x14,0x14,0x14,0x14,0x14,0x12,0x10,0x10,0x10};
					HI_U8 SharpenAltUd[] =  {120,120,120,100,60,48,20,20,20,20,20,0x12,0x10,0x10,0x10,0x10};
					HI_U8 au8SharpenRGB[]=  {105,96,86,75,50,45,25,25,20,0x30,0x20,0x16,0x12,0x12,0x12,0x12};
				
				
					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

				}
				if(sensorid == SENSOR_AR0330)
				{
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

				}
				if(sensorid == SENSOR_IMX290)
				{
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);

				
					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 110*1024;
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

					//HI_U8 SharpenAltD[]  = {70,70,90,90,80,70,70,70,60,35,35,0x12,0x12,0x12,0x12,0x12};
					//HI_U8 SharpenAltUd[] =  {120,110,105,85,70,70,50,35,30,30,0x20,0x16,0x12,0x12,0x12,0x12};

					HI_U8 SharpenAltD[]  = {70,70,70,80,84,84,120,130,130,120,100,0x12,0x12,0x12,0x12,0x12};
					HI_U8 SharpenAltUd[] =  {120,115,105,87,75,65,50,40,30,20,20,20,0x12,0x12,0x12,0x12};
					
					HI_U8 au8SharpenRGB[]=  {78,88,82,75,70,55,45,30,16,16,16,16,0x20,0x20,0x20,0x20};


					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					
					
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

				

				}
				if(sensorid == SENSOR_IMX123)
				{
					jv_sensor_wdr_switch(FALSE);
					isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
					
					ISP_GAMMA_ATTR_S pstGammaAttr;
					pstGammaAttr.bEnable = HI_TRUE;
					pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
					memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);
					
					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 72*1024; 
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


					
					HI_U8 SharpenAltD[]  = {100,100,100,100,100,110,100,70,65,60,50,0x8,0x8,0x8,0x8,0x8};
					HI_U8 SharpenAltUd[] = {98,96,78,70,70,65,55,55,16,12,10,8,8,8,0x10,0x10};
					HI_U8 au8SharpenRGB[]={70,65,60,55,48,40,32,20,0xC,0xA,0xA,0xA,0xA,0xA,0xA,0xA};
				
				
					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);
				}

            }
            else if(CHECK_DAY == value)
            {
            	//entering day mode
            	nightflag = HI_FALSE;
				
				if(sensorid == SENSOR_OV4689)
				{
					if(bUserWDROpen)//if wdr enabled,disable it
					{
						jv_sensor_wdr_switch(TRUE);
					}
					else // linear 
					{
						//HI_U8 SharpenAltD[]  = {60,61,58,77,66,61,60,40,30,30,30,30,30,30,30,30};
						//HI_U8 SharpenAltUd[] = {96,96,88,60,50,42,42,40,40,40,40,40,20,20,20,20};
						//HI_U8 au8SharpenRGB[]={136,128,120,90,76,60,50,50,50,50,50,50,50,50,50,50};
						
						HI_U8 SharpenAltD[]  = {72,80,80,80,90,95,110,110,100,30,30,30,30,30,30,30};
						HI_U8 SharpenAltUd[] = {96,86,82,65,45,40,30,30,30,40,40,40,20,20,20,20};
						HI_U8 au8SharpenRGB[]={132,105,100,78,76,60,50,50,50,50,50,50,50,50,50,50};
				
				
						ISP_SHARPEN_ATTR_S  pstSharpenAttr;
						HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
						memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
						memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
						memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
						HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);


					}
				}
				if(sensorid == SENSOR_IMX123)
				{
					if(bUserWDROpen)//if wdr enabled,disable it
					{
						jv_sensor_wdr_switch(TRUE);
					}
					else
					{
						ISP_EXPOSURE_ATTR_S stExpAttr;
						HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
						stExpAttr.stAuto.stSysGainRange.u32Max = 512*1024; 
						HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);



						HI_U8 SharpenAltD[]  = {85,82,80,80,90,100,90,70,65,60,50,0x8,0x8,0x8,0x8,0x8};
						HI_U8 SharpenAltUd[] = {98,96,78,70,60,55,45,25,16,12,10,8,8,8,0x10,0x10};
						HI_U8 au8SharpenRGB[]={70,65,60,55,48,40,32,20,0xC,0xA,0xA,0xA,0xA,0xA,0xA,0xA};
				
				
						ISP_SHARPEN_ATTR_S  pstSharpenAttr;
						HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
						memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
						memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
						memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
						HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);
					}
				}
				
				if(sensorid == SENSOR_AR0230||sensorid ==SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
				{
					if(bUserWDROpen)//if wdr enabled,disable it
					{
						jv_sensor_wdr_switch(TRUE);
					}
					else
					{
						if(sensorid ==SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
						{
							HI_U8 SharpenAltD[]  = {62,62,60,90,90,115,85,55,0x22,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
							HI_U8 SharpenAltUd[] = {123,117,107,75,56,32,28,20,16,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
							HI_U8 au8SharpenRGB[]= {113,113,103,80,65,45,32,28,24,0x10,0x10,0x10,0x10,0x10,0x10,0x10};

							ISP_SHARPEN_ATTR_S  pstSharpenAttr;
							HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
							memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
							memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
							memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
							HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);
							ISP_EXPOSURE_ATTR_S stExpAttr;
							HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
							stExpAttr.stAuto.stSysGainRange.u32Max = 32*1024; 
							HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
						}
						if(sensorid== SENSOR_AR0230)
						{
							HI_U8 SharpenAltD[]  = {45,50,55,75,110,110,110,80,0x22,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
							HI_U8 SharpenAltUd[] =  {120,120,90,80,65,55,45,35,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
							HI_U8 au8SharpenRGB[]=  {110,110,115,110,90,65,50,40,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
				
				
							ISP_SHARPEN_ATTR_S  pstSharpenAttr;
							HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
							memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
							memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
							memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
							HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);


							ISP_EXPOSURE_ATTR_S stExpAttr;
							HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
							stExpAttr.stAuto.stSysGainRange.u32Max = 75*1024; 
							HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


						}
					}
					
			
				}
				if(sensorid==SENSOR_IMX290)
				{

					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					//stExpAttr.stAuto.stSysGainRange.u32Max = 360*1024;		
					stExpAttr.stAuto.stSysGainRange.u32Max = 400*1024;	
					if(__check_cp())
					{
						stExpAttr.stAuto.stSysGainRange.u32Max = 100*1024;
						stExpAttr.stAuto.stExpTimeRange.u32Max = 7500;
						stExpAttr.stAuto.u8Tolerance = 3;
						stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 10;
						stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
						stExpAttr.stAuto.u8Speed = 64;
						stExpAttr.stAuto.u8MaxHistOffset = 20;
					}
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

					//HI_U8 SharpenAltD[]  = {70,70,90,90,80,70,70,70,60,35,35,0x12,0x12,0x12,0x12,0x12};
					//HI_U8 SharpenAltUd[] =  {120,110,105,85,70,70,50,35,30,30,0x20,0x16,0x12,0x12,0x12,0x12};

					HI_U8 SharpenAltD[]  = {70,70,70,80,84,84,120,130,130,120,100,0x12,0x12,0x12,0x12,0x12};
					HI_U8 SharpenAltUd[] =  {120,115,105,87,75,65,50,40,30,20,20,20,0x12,0x12,0x12,0x12};
					
					HI_U8 au8SharpenRGB[]=  {78,88,82,75,70,55,45,30,16,16,16,16,0x20,0x20,0x20,0x20};


					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					
					
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

						
				}
				if(sensorid == SENSOR_OV2710)
				{
					ISP_EXPOSURE_ATTR_S stExpAttr;
					HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
					stExpAttr.stAuto.stSysGainRange.u32Max = 30*1024; 
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

					
					HI_U8 SharpenAltD[]  =  {65,63,62,68,68,62,50,0x14,0x14,0x14,0x14,0x14,0x12,0x10,0x10,0x10};
					HI_U8 SharpenAltUd[] =  {120,120,120,90,60,32,20,20,20,20,20,0x12,0x10,0x10,0x10,0x10};
					HI_U8 au8SharpenRGB[]=  {105,96,86,75,50,40,25,25,20,0x30,0x20,0x16,0x12,0x12,0x12,0x12};
				
				
					ISP_SHARPEN_ATTR_S  pstSharpenAttr;
					HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
					memcpy(pstSharpenAttr.stAuto.au8SharpenD,SharpenAltD,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenUd,SharpenAltUd,16);
					memcpy(pstSharpenAttr.stAuto.au8SharpenRGB,au8SharpenRGB,16);
					HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);
				}
				isp_ioctl(0,ADJ_BRIGHTNESS,user_set_luma);
				isp_ioctl(0,ADJ_CONTRAST,user_set_contrast);
				
            }
            else if(DEFAULT == value)
            {
 
            }
            else if(MODE1== value)
            {

            }
			break;
		}
        case ADJ_BRIGHTNESS://设置亮度
             {

                if(value <0||value > 255)
                    return -1;
				user_set_luma  = value;
				ISP_EXPOSURE_ATTR_S stExpAttr;
				HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
				stExpAttr.stAuto.u8Compensation = 64;

				if(jv_sensor_get_wdr_mode()==1)//宽动态亮度设置 
				{
					signed int value_cal =64;
					if(sensorid == SENSOR_OV4689)
					{
						if(nightflag)
							value_cal = user_set_luma -OV4689_WDR_NIGHT_LUM;
						else
							value_cal = user_set_luma -OV4689_WDR_DAY_LUM;
					}
					else if(sensorid == SENSOR_IMX123)
					{
						if(nightflag)
							value_cal = user_set_luma -OV4689_WDR_NIGHT_LUM;
						else
							value_cal = user_set_luma -OV4689_WDR_DAY_LUM;
					}
					else if(sensorid == SENSOR_AR0230)
					{
						if(__check_cp())
							return 0;
							
						if(nightflag)
							value_cal = user_set_luma -AR0230_WDR_NIGHT_LUM;
						else
							value_cal = user_set_luma -AR0230_WDR_DAY_LUM;
					}
					else if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
					{
					
						if(nightflag)
							value_cal = user_set_luma -AR0237_WDR_NIGHT_LUM;
						else
							value_cal = user_set_luma -AR0237_WDR_DAY_LUM;
					}
					if(value_cal<0)
						value_cal =0;
					stExpAttr.stAuto.u8Compensation = value_cal;
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
						
				}
				else if (jv_sensor_get_wdr_mode()==0)
				{
					signed int value_cal =64;
					if(sensorid == SENSOR_OV4689||sensorid == SENSOR_IMX178)
					{
						if(nightflag)		
							value_cal = user_set_luma -OV4689_LINEAR_NIGHT_LUM;
						else
							value_cal = user_set_luma -OV4689_LINEAR_DAY_LUM;
					}
					else if(sensorid == SENSOR_AR0230||sensorid == SENSOR_AR0330)
					{
						if(nightflag)		
							value_cal = user_set_luma -AR0230_LINEAR_NIGHT_LUM;
						else
							value_cal = user_set_luma -AR0230_LINEAR_DAY_LUM;
					}
					else if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
					{
					
						if(nightflag)
							value_cal = user_set_luma -AR0237_LINEAR_NIGHT_LUM;
						else
							value_cal = user_set_luma -AR0237_LINEAR_DAY_LUM;
					}
					else if(sensorid == SENSOR_OV2710)
					{
					
						if(nightflag)
							value_cal = user_set_luma -OV2710_LINEAR_NIGHT_LUM;
						else
							value_cal = user_set_luma -OV2710_LINEAR_DAY_LUM;
					}
					else if(sensorid == SENSOR_IMX290)
					{
						if(nightflag)		
							value_cal = user_set_luma -IMX290_NIGHT_LUM;
						else
							value_cal = user_set_luma -IMX290_DAY_LUM;
						if(__check_cp())
						{
							value_cal = user_set_luma - CP_LINEAR_LUM;

						}
					}
					else if(sensorid == SENSOR_IMX123)
					{
						if(nightflag)		
							value_cal = user_set_luma -IMX123_NIGHT_LUM;
						else
							value_cal = user_set_luma -IMX123_DAY_LUM;
					}
					else if(sensorid == SENSOR_IMX185)
					{
						if(nightflag)
							value_cal = user_set_luma -IMX185_LINEAR_NIGHT_LUM;
						else
							value_cal = user_set_luma -IMX185_LINEAR_DAY_LUM;
						if(__check_cp())
						{
							value_cal = user_set_luma - CP_LINEAR_LUM;

						}
					}
					if(value_cal<0)
						value_cal =0;
					stExpAttr.stAuto.u8Compensation = value_cal;
					HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
				}
				
                break;
             }
        case ADJ_SATURATION://设置饱和度
             {
			 	signed int auSat[16];
				signed int tmpSat;
                if(value >255 || value <0)
                    return -1;

				ISP_SATURATION_ATTR_S pstSatAttr;
				HI_MPI_ISP_GetSaturationAttr(IspDev, &pstSatAttr);
				pstSatAttr.enOpType = OP_TYPE_AUTO;
				//pstSatAttr.stManual.u8Saturation = value;

				if(sensorid == SENSOR_IMX178)
				{
					 tmpSat =  value;
					 auSat[0] = tmpSat-0;       //128
					 auSat[1] = tmpSat-0;       //128
					 auSat[2] = tmpSat-5;     //118
					 auSat[3] = tmpSat-14;    //108
					 auSat[4] = tmpSat-24;    //88
					 auSat[5] = tmpSat-34;    //78
					 auSat[6] = tmpSat-40;    //68 
					 auSat[7] = tmpSat-48;    //68
					 auSat[8] = tmpSat-56;    //68
					 auSat[9] = tmpSat-56;    //68 
					 auSat[10] = tmpSat-56;    //68
					 auSat[11] = tmpSat-56;    //68
					 auSat[12] = tmpSat-56;    //68
					 auSat[13] = tmpSat-56;    //68
					 auSat[14] = tmpSat-56;    //68
					 auSat[15] = tmpSat-56;    //68	   

				}
				else if(sensorid == SENSOR_AR0330)
				{
					 tmpSat =  value;
					 auSat[0] = tmpSat-0;       //128
					 auSat[1] = tmpSat-0;       //128
					 auSat[2] = tmpSat-5;     //118
					 auSat[3] = tmpSat-14;    //108
					 auSat[4] = tmpSat-24;    //88
					 auSat[5] = tmpSat-34;    //78
					 auSat[6] = tmpSat-40;    //68 
					 auSat[7] = tmpSat-48;    //68
					 auSat[8] = tmpSat-56;    //68
					 auSat[9] = tmpSat-56;    //68 
					 auSat[10] = tmpSat-56;    //68
					 auSat[11] = tmpSat-56;    //68
					 auSat[12] = tmpSat-56;    //68
					 auSat[13] = tmpSat-56;    //68
					 auSat[14] = tmpSat-56;    //68
					 auSat[15] = tmpSat-56;    //68	   

				}
				else if(sensorid == SENSOR_OV2710)
				{
					 tmpSat =  value;
					 auSat[0] = tmpSat;       //128
					 auSat[1] = tmpSat;       //128
					 auSat[2] = tmpSat;     //118
					 auSat[3] = tmpSat-10;    //108
					 auSat[4] = tmpSat-32;    //88
					 auSat[5] = tmpSat-50;    //78
					 auSat[6] = tmpSat-75;    //68 
					 auSat[7] = tmpSat-80;    //68
					 auSat[8] = tmpSat-80;    //68
					 auSat[9] = tmpSat-80;    //68 
					 auSat[10] = tmpSat-80;    //68
					 auSat[11] = tmpSat-80;    //68
					 auSat[12] = tmpSat-80;    //68
					 auSat[13] = tmpSat-80;    //68
					 auSat[14] = tmpSat-80;    //68
					 auSat[15] = tmpSat-80;    //68	   

				}
				else if(sensorid == SENSOR_OV4689)
				{
					 tmpSat =  value;
					 if(jv_sensor_get_wdr_mode()==0)
					 {
					 	auSat[0] = tmpSat-0;       //128
					 	auSat[1] = tmpSat-0;       //128
					 	auSat[2] = tmpSat-2;     //118
					 	auSat[3] = tmpSat-14;    //108
					 	auSat[4] = tmpSat-44;    //88
					 	auSat[5] = tmpSat-52;    //78
					 	auSat[6] = tmpSat-60;    //68 
					 	auSat[7] = tmpSat-68;    //68
					 	auSat[8] = tmpSat-76;    //68
					 	auSat[9] = tmpSat-76;    //68 
					 	auSat[10] = tmpSat-72;    //68
					 	auSat[11] = tmpSat-72;    //68
					 	auSat[12] = tmpSat-72;    //68
					 	auSat[13] = tmpSat-72;    //68
					 	auSat[14] = tmpSat-72;    //68
					 	auSat[15] = tmpSat-72;    //68
					 }
					 else if(jv_sensor_get_wdr_mode()==1)
					 {
					 	auSat[0] = tmpSat-0;       //128
					 	auSat[1] = tmpSat-0;       //128
					 	auSat[2] = tmpSat-0;     //118
					 	auSat[3] = tmpSat-8;    //108
					 	auSat[4] = tmpSat-16;    //88
					 	auSat[5] = tmpSat-30;    //78
					 	auSat[6] = tmpSat-40;    //68 
					 	auSat[7] = tmpSat-56;    //68
					 	auSat[8] = tmpSat-64;    //68
					 	auSat[9] = tmpSat- 72;    //68 
					 	auSat[10] = tmpSat-72;    //68
					 	auSat[11] = tmpSat-72;    //68
					 	auSat[12] = tmpSat-72;    //68
					 	auSat[13] = tmpSat-72;    //68
					 	auSat[14] = tmpSat-72;    //68
					 	auSat[15] = tmpSat-72;    //68	
					 }

				}
				else if(sensorid == SENSOR_IMX185)
				{
					 tmpSat =  value;
					 if(jv_sensor_get_wdr_mode()==0)
					 {
					 	auSat[0] = tmpSat+12;       
					 	auSat[1] = tmpSat+10;       
					 	auSat[2] = tmpSat+5;     
					 	auSat[3] = tmpSat-0;    
					 	auSat[4] = tmpSat-0;    
					 	auSat[5] = tmpSat-10;   
					 	auSat[6] = tmpSat-24;  //64x 108  
					 	
					 	auSat[7] = tmpSat-40;    //128x
					 	auSat[8] = tmpSat-56;    //256x
					 	auSat[9] = tmpSat-64;    //68 
					 	auSat[10] = tmpSat-72;    //68
					 	auSat[11] = tmpSat-72;    //68
					 	auSat[12] = tmpSat-72;    //68
					 	auSat[13] = tmpSat-72;    //68
					 	auSat[14] = tmpSat-72;    //68
					 	auSat[15] = tmpSat-72;    //68
					 }
					 else if(jv_sensor_get_wdr_mode()==1)
					 {
						 auSat[0] = tmpSat+10;		
						 auSat[1] = tmpSat+10;		
						 auSat[2] = tmpSat+5;	  
						 auSat[3] = tmpSat+0;	 
						 auSat[4] = tmpSat-8;	 
						 auSat[5] = tmpSat-15;	 
						 auSat[6] = tmpSat-32;	//64x  108	
						 
						 auSat[7] = tmpSat-24;	  //128x
						 auSat[8] = tmpSat-46;	  //256x
						 auSat[9] = tmpSat-64;	  //68 
						 auSat[10] = tmpSat-72;    //68
						 auSat[11] = tmpSat-72;    //68
						 auSat[12] = tmpSat-72;    //68
						 auSat[13] = tmpSat-72;    //68
						 auSat[14] = tmpSat-72;    //68
						 auSat[15] = tmpSat-72;    //68

					 }

				}

				else if(sensorid == SENSOR_AR0230||sensorid == SENSOR_AR0237DC)
				{
					 tmpSat =  value;
					 if(jv_sensor_get_wdr_mode()==0)
					 {
						 tmpSat =  value;
						 auSat[0] = tmpSat-0;		//128
						 auSat[1] = tmpSat-0;		//128
						 auSat[2] = tmpSat-2;	  //118
						 auSat[3] = tmpSat-14;	  //108
						 auSat[4] = tmpSat-24;	  //88
						 auSat[5] = tmpSat-32;	  //78
						 auSat[6] = tmpSat-40;	  //68 
						 auSat[7] = tmpSat-48;	  //68
						 auSat[8] = tmpSat-56;	  //68
						 auSat[9] = tmpSat-64;	  //68 
						 auSat[10] = tmpSat-72;    //68
						 auSat[11] = tmpSat-72;    //68
						 auSat[12] = tmpSat-72;    //68
						 auSat[13] = tmpSat-72;    //68
						 auSat[14] = tmpSat-72;    //68
						 auSat[15] = tmpSat-72;    //68 

					 }
					 else if(jv_sensor_get_wdr_mode()==1)
					 {
						 tmpSat =  value;
						 auSat[0] = tmpSat-0;		//128
						 auSat[1] = tmpSat-0;		//128
						 auSat[2] = tmpSat-0;	  //118
						 auSat[3] = tmpSat-10;	  //108
						 auSat[4] = tmpSat-16;	  //88
						 auSat[5] = tmpSat-28;	  //78
						 auSat[6] = tmpSat-32;	  //68 
						 auSat[7] = tmpSat-40;	  //68
						 auSat[8] = tmpSat-56;	  //68
						 auSat[9] = tmpSat-64;	  //68 
						 auSat[10] = tmpSat-72;    //68
						 auSat[11] = tmpSat-72;    //68
						 auSat[12] = tmpSat-72;    //68
						 auSat[13] = tmpSat-72;    //68
						 auSat[14] = tmpSat-72;    //68
						 auSat[15] = tmpSat-72;    //68 

					 }

				
				}
				else if(sensorid == SENSOR_IMX290)
				{
					HI_U8 Sat_def[16]= {0x80,0x80,0x80,0x80,0x80,0x65,0x60,0x50,56,48,48,48,0x36,0x36,0x36,0x36};
					int i;
					signed int CalTmp=0;
					tmpSat =  value;
					for(i=0;i<16;i++)
					{
						if(Sat_def[i]>=128)	
							auSat[i] = tmpSat + (Sat_def[i]-128);
						else
							auSat[i] = tmpSat -(128-Sat_def[i]);
					}
				}
				else if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
				{
					HI_U8 Sat_def_line[16]= {0x80,0x80,0x7e,0x72,0x68,0x60,0x58,0x50,0x48,0x40,0x38,0x38,0x38,0x38,0x38,0x38};
					HI_U8 Sat_def_wdr[16]=  {0x85,0x85,0x7e,0x72,0x68,0x60,0x58,0x50,0x48,0x40,0x38,0x38,0x38,0x38,0x38,0x38};
					
					int i;
					signed int CalTmp=0;
					tmpSat =  value;
					for(i=0;i<16;i++)
					{
						if(Sat_def_line[i]>=128)	
							auSat[i] = tmpSat + (Sat_def_line[i]-128);
						else
							auSat[i] = tmpSat -(128-Sat_def_line[i]);
					}
					if(jv_sensor_get_wdr_mode()==1)
					{
						for(i=0;i<16;i++)
						{
							if(Sat_def_wdr[i]>=128)	
								auSat[i] = tmpSat + (Sat_def_wdr[i]-128);
							else
								auSat[i] = tmpSat -(128-Sat_def_wdr[i]);
						}
					}
				}
				else if(sensorid == SENSOR_IMX123)
				{
					//HI_U8 Sat_def[16]= {0x80,0x80,0x80,0x7e,0x7b,0x78,0x69,0x50,0x43,0x40,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a};
					HI_U8 Sat_def[16]= {128,128,128,126,115,100,78,66,48,40,40,40,40,40,40,40};
					int i;
					signed int CalTmp=0;
					tmpSat =  value;
					for(i=0;i<16;i++)
					{
						if(Sat_def[i]>=128)	
							auSat[i] = tmpSat + (Sat_def[i]-128);
						else
							auSat[i] = tmpSat -(128-Sat_def[i]);
					}
				}
				else
				{
					 tmpSat =  value;
					 auSat[0] = tmpSat-0;       //128
					 auSat[1] = tmpSat-0;       //128
					 auSat[2] = tmpSat-2;     //118
					 auSat[3] = tmpSat-14;    //108
					 auSat[4] = tmpSat-24;    //88
					 auSat[5] = tmpSat-32;    //78
					 auSat[6] = tmpSat-40;    //68 
					 auSat[7] = tmpSat-48;    //68
					 auSat[8] = tmpSat-56;    //68
					 auSat[9] = tmpSat-64;    //68 
					 auSat[10] = tmpSat-72;    //68
					 auSat[11] = tmpSat-72;    //68
					 auSat[12] = tmpSat-72;    //68
					 auSat[13] = tmpSat-72;    //68
					 auSat[14] = tmpSat-72;    //68
					 auSat[15] = tmpSat-72;    //68	
				}
				
				 int i;
				 for(i=0;i<16;i++)
				 {
						if( auSat[i]>255)
							auSat[i]=255;
						if( auSat[i]<0)
							auSat[i]=0;	
						pstSatAttr.stAuto.au8Sat[i] = (HI_U8)auSat[i];
				 }
					 
				
				if(jv_sensor_get_wdr_mode()>=0)
					HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);
                break;
             }
        case ADJ_CONTRAST://设置对比度
             {
          
                if(value >255 || value <0)
                    return -1;
				user_set_contrast =value;
				if(nightflag)
					return 0;
				ISP_GAMMA_ATTR_S pstGammaAttr;
				pstGammaAttr.bEnable = HI_TRUE;
				pstGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
				if(sensorid == SENSOR_IMX178)
				{
					
					if(value > 200)
				  	{
						user_select_gamma =3;
					  	memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
				   	}
				   	else if(value < 50)
				   	{
					   	memcpy(&pstGammaAttr.u16Table,&Infrared_default_gamma,257 * 2);
						user_select_gamma =1;
				   	}
				   	else
				   	{
					   	memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
					   	user_select_gamma =2;
				   	}

				}
				else if(sensorid == SENSOR_IMX185)
				{
					if(jv_sensor_get_wdr_mode()==0)
					{
						if(value > 200)
				   		{
							user_select_gamma =3;
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   		}
				   		else if(value < 50)
				   		{
					  		 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
								user_select_gamma =1;
				   		}
				   		else
				   		{
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					   		user_select_gamma =2;
				   		}
					}
					else if(jv_sensor_get_wdr_mode()==1)
					{
						if(value < 50)
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_l_gamma,257 * 2);
							user_select_gamma =6;
						}
						else if(value >200)
						{
							memcpy(&pstGammaAttr.u16Table,&u16Gamma_wdr_h,257 * 2);
							user_select_gamma =5;
						}
						else
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_h_gamma,257 * 2);
							user_select_gamma =4;


						}



					}

				}
			    else if(sensorid == SENSOR_IMX290||sensorid == SENSOR_AR0330)
				{
					
					if(value > 200)
				   	{
						user_select_gamma =3;
					   	memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   	}
				   	else if(value < 50)
				   	{
					  	memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
						user_select_gamma =1;
				   	}
				   	else
				   	{
					   	memcpy(&pstGammaAttr.u16Table,&OV4689_LINE_GAMMA,257 * 2);
					   	user_select_gamma =2;
				   	}

				}
				else if(sensorid == SENSOR_IMX123)
				{
					
						if(value >=200)
				  		{
							user_select_gamma =3;
					  		// memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
					  		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   		}
				   		else if(value <=50)
				   		{
					   		//memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
					   		 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
							user_select_gamma =1;
				   		}
				   		else
				   		{
					   		//memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					   		memcpy(&pstGammaAttr.u16Table,&OV4689_LINE_GAMMA,257 * 2);
					   		user_select_gamma =2;							
				   		}
						if(jv_sensor_get_wdr_mode()==1)
						{
							if(value < 50)
							{
								memcpy(&pstGammaAttr.u16Table,&FSWDR_l_gamma,257 * 2);
								user_select_gamma =6;
							}
							else if(value >200)
							{
								memcpy(&pstGammaAttr.u16Table,&u16Gamma_wdr_h,257 * 2);
								user_select_gamma =5;
							}
							else
							{
								memcpy(&pstGammaAttr.u16Table,&IMX123_WDR_GAMMA,257 * 2);
								user_select_gamma =4;
							}

						}

				}
				else if(sensorid == SENSOR_OV4689)
				{
					if(jv_sensor_get_wdr_mode()==0)
					{
						if(value > 200)
				   		{
							user_select_gamma =3;
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   		}
				   		else if(value < 50)
				   		{
					  		 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
								user_select_gamma =1;
				   		}
				   		else
				   		{
					   		memcpy(&pstGammaAttr.u16Table,&OV4689_LINE_GAMMA,257 * 2);
					   		user_select_gamma =2;
				   		}
					}
					else if(jv_sensor_get_wdr_mode()==1)
					{
						if(value < 50)
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_l_gamma,257 * 2);
							user_select_gamma =6;
						}
						else if(value >200)
						{
							memcpy(&pstGammaAttr.u16Table,&u16Gamma_wdr_h,257 * 2);
							user_select_gamma =5;
						}
						else
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_h_gamma,257 * 2);
							user_select_gamma =4;


						}



					}

				}
				else if(sensorid == SENSOR_AR0230)
				{
					if(jv_sensor_get_wdr_mode()==0)
					{
						if(value > 200)
				   		{
							user_select_gamma =3;
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   		}
				   		else if(value < 50)
				   		{
					  		 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
								user_select_gamma =1;
				   		}
				   		else
				   		{
					   		memcpy(&pstGammaAttr.u16Table,&OV4689_LINE_GAMMA,257 * 2);
					   		user_select_gamma =2;
				   		}
					}
					else if(jv_sensor_get_wdr_mode()==1)
					{
						if(value < 50)
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_l_gamma,257 * 2);
							user_select_gamma =6;
						}
						else if(value >200)
						{
							memcpy(&pstGammaAttr.u16Table,&u16Gamma_wdr_h,257 * 2);
							user_select_gamma =5;
						}
						else
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_h_gamma,257 * 2);
							user_select_gamma =4;


						}



					}

				}
				else if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
				{
					if(jv_sensor_get_wdr_mode()==0)
					{
						if(value > 200)
				   		{
							user_select_gamma =3;
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   		}
				   		else if(value < 50)
				   		{
					  		 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
								user_select_gamma =1;
				   		}
				   		else
				   		{
					   		memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					   		user_select_gamma =2;
				   		}
					}
					else if(jv_sensor_get_wdr_mode()==1)
					{
						if(value < 50)
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_l_gamma,257 * 2);
							user_select_gamma =6;
						}
						else if(value >200)
						{
							memcpy(&pstGammaAttr.u16Table,&u16Gamma_wdr_h,257 * 2);
							user_select_gamma =5;
						}
						else
						{
							memcpy(&pstGammaAttr.u16Table,&FSWDR_h_gamma,257 * 2);
							user_select_gamma =4;


						}



					}

				}
				else if(sensorid == SENSOR_OV2710)
				{

					
					if(value > 200)
				   	{
						user_select_gamma =3;
					   	memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_H_16A,257 * 2);
				   	}
				   	else if(value < 50)
				   	{
					  	 memcpy(&pstGammaAttr.u16Table,&IMX178_default_gamma,257 * 2);
						 user_select_gamma =1;
				   	}
				   	else
				   	{
					   	memcpy(&pstGammaAttr.u16Table,&OV2710_LINE_GAMMA_16A,257 * 2);
					   	user_select_gamma =2;
				   	}
					


				}
								
				int ret;
				if(jv_sensor_get_wdr_mode()>=0)
				{
					ret=HI_MPI_ISP_SetGammaAttr(IspDev,&pstGammaAttr);
				}
				else
					printf("running\n");
								
                break;
             }
         case  ADJ_COLOURMODE:
             {
			 	ISP_SATURATION_ATTR_S pstSatAttr;
			 	HI_MPI_ISP_GetSaturationAttr(IspDev, &pstSatAttr);
                if(value == 1)//彩色模式
                {
                    
                    pstSatAttr.enOpType = OP_TYPE_AUTO;
                    HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);
                }
                else           //黑白色
                {
                   
                    pstSatAttr.enOpType = OP_TYPE_MANUAL;
                    pstSatAttr.stManual.u8Saturation = 0x0;
                    HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);
                }
                break;
             }
         case ADJ_SHARPNESS:
             {
			 	 signed int uu_value=-1; 
				 if(sensorid == SENSOR_IMX178)
				 {
				 	uu_value = value;
					if(nightflag)
				 		uu_value += 52; 
					else
						uu_value += 52; 
				 }
				 else if(sensorid == SENSOR_IMX185)
				 {
				 	uu_value = value;
					if(nightflag)
				 		uu_value += 52; 
					else
						uu_value += 52; 
					uu_value = -1;
				 }
				 else if(sensorid == SENSOR_OV4689)
				 {
				 	uu_value = value;
					if(strstr(hwinfo.product,"STC"))
					{
						if(nightflag)
							uu_value += 46; 
						else
							uu_value += 46; 
					}
					else
					{
						if(nightflag)
							uu_value += 56; 
						else
							uu_value += 56; 
					}
				 }
				// else if(sensorid == SENSOR_AR0230||sensorid == SENSOR_AR0330)
				// {
				 	//uu_value = value;
					//if(nightflag)
				 		//uu_value += 32; 
					//else
						//uu_value += 32; 
				 //}
				else if(sensorid == SENSOR_AR0330)
				{
					uu_value = value;
					if(nightflag)
						uu_value += 32;
					else
						uu_value += 52; 
				}
				else if(sensorid == SENSOR_IMX123)
				{
					uu_value = value;
					if(nightflag)
						uu_value += 45;
					else
						uu_value += 45; 
				}
				else if(sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
				{
					uu_value = value;
					if(nightflag)
						uu_value += 32;
					else
						uu_value += 32; 
				}
				else if(sensorid == SENSOR_OV2710)
				{
					uu_value = value;
					if(nightflag)
						uu_value += 62;
					else
						uu_value += 62; 
				}


				 if(uu_value>=0)
				 {
				 	if(uu_value > 255)
						uu_value = 255;
					
				 	ISP_DEMOSAIC_ATTR_S pstDemosaicAttr;
				 
				 	HI_MPI_ISP_GetDemosaicAttr(IspDev, &pstDemosaicAttr);
				 	pstDemosaicAttr.u8UuSlope = uu_value;
				 	HI_MPI_ISP_SetDemosaicAttr(IspDev, &pstDemosaicAttr);
				 }
                 break;
             }
        case SFHDR_SW:
             {
					printf("SFHDR_SW setting \n");
                    break;
             }
        case MIRROR_TURN:
                {
                    if(value==0)
                        VI_Mirror_Flip(HI_FALSE,HI_FALSE);
                    if(value==1)
                        VI_Mirror_Flip(HI_TRUE,HI_FALSE);
                    if(value==2)
                        VI_Mirror_Flip(HI_FALSE,HI_TRUE);
                    if(value==3)
                        VI_Mirror_Flip(HI_TRUE,HI_TRUE);
                    break;
                }
        case PFI_SW:
                {
                    if(value==PFI_ON)
                        {/*
                            ISP_IMAGE_ATTR_S pstImageAttr;
                            HI_MPI_ISP_GetImageAttr(IspDev, &pstImageAttr);
                            pstImageAttr.u16FrameRate =25;
                            HI_MPI_ISP_SetImageAttr(IspDev, &pstImageAttr);*/
                        }
                    if(value == PFI_OFF)
                        {/*
                            ISP_IMAGE_ATTR_S pstImageAttr;
                            HI_MPI_ISP_GetImageAttr(IspDev, &pstImageAttr);
                            pstImageAttr.u16FrameRate =30;
                            HI_MPI_ISP_SetImageAttr(IspDev, &pstImageAttr);*/
                        }
                    break;
                }
               case LOW_FRAME:
                {
                    //printf("HI_MPI_ISP_SetSlowFrameRate %d\n",value);
                    //HI_MPI_ISP_SetSlowFrameRate(value);
                    break;
                }
        case WDR_SW:
                {

					if (sensorid == SENSOR_IMX178||sensorid ==SENSOR_IMX290||sensorid ==SENSOR_AR0330||sensorid == SENSOR_OV2710||sensorid == SENSOR_IMX185)
					{
							ISP_DRC_ATTR_S pstDRCAttr;
                            HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
							if(value ==1)
								pstDRCAttr.bEnable = HI_TRUE;
							else
								pstDRCAttr.bEnable = HI_FALSE;
                            HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);      //关闭软件HDR

					}
					else if(sensorid == SENSOR_IMX123)
					{
						
						if(strstr(hwinfo.devName, "N52-HS"))
						{
							if(value==0) //关闭wdr
							{
								bUserWDROpen =FALSE;
								jv_sensor_wdr_switch(FALSE);
								
							}
							else
							{
								bUserWDROpen =TRUE;
								jv_sensor_wdr_switch(TRUE);
							}
						}
						else
						{
							
							ISP_DRC_ATTR_S pstDRCAttr;
                            HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
							if(value ==1)
								pstDRCAttr.bEnable = HI_TRUE;
							else
								pstDRCAttr.bEnable = HI_FALSE;
                            HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);      //关闭软件HDR
						}
					}
					else if (sensorid == SENSOR_OV4689||sensorid == SENSOR_AR0230||sensorid == SENSOR_AR0237||sensorid == SENSOR_AR0237DC)
					{
						
						if(value==0) //关闭wdr
						{
							bUserWDROpen =FALSE;
							jv_sensor_wdr_switch(FALSE);
								
						}
						else
						{
							bUserWDROpen =TRUE;
							jv_sensor_wdr_switch(TRUE);

						}
						
					}
					else
					{
							ISP_DRC_ATTR_S pstDRCAttr;
                            HI_MPI_ISP_GetDRCAttr(IspDev, &pstDRCAttr);
							if(value ==1)
								pstDRCAttr.bEnable = HI_TRUE;
							else
								pstDRCAttr.bEnable = HI_FALSE;
                            HI_MPI_ISP_SetDRCAttr(IspDev, &pstDRCAttr);      //关闭软件HDR

					}
                    break;
                }
#endif
        case GET_ID:
                {
					
					int ret;
					if(sensorType != SENSOR_UNKNOWN)
					{
						*(int *)value = sensorid =sensorType;
						return 0;
					}

					
					ret = reg_i2c_read(0x42,0x300A,2,1);
					if(ret==0x46)
					{

						*(int *)value = SENSOR_OV4689;
                    	sensorType =sensorid = SENSOR_OV4689;
						printf(">>>>check sensor ID : OV4689<<<<\n");
						return 0;
					}
					ret = reg_i2c_read(0x6c,0x300A,2,1);
					if(ret==0x27)
					{

						*(int *)value = SENSOR_OV2710;
                    	sensorType =sensorid = SENSOR_OV2710;
						printf(">>>>check sensor ID : OV2710<<<<\n");
						return 0;
					}

					ret = reg_i2c_read(0x30,0x3000,2,2);
					if(ret == 0x2604)
					{
						*(int *)value = SENSOR_AR0330;
						sensorType =sensorid = SENSOR_AR0330;
						printf(">>>>check sensor ID : AR0330<<<<\n");
						return 0;
					}
					ret = reg_i2c_read(0x20,0x3000,2,2);
					if(ret == 0x56)
					{
						*(int *)value = SENSOR_AR0230;
						sensorType =sensorid = SENSOR_AR0230;
						printf(">>>>check sensor ID : AR0230<<<<\n");
						return 0;
					}
					ret = reg_i2c_read(0x20,0x3000,2,2);
					if(ret == 0x256)
					{
                        if(AR0237DC)
                        {
                            *(int *)value = SENSOR_AR0237DC;
						    sensorType =sensorid = SENSOR_AR0237DC;
						    printf(">>>>check sensor ID : AR0237DC<<<<\n");
						    return 0;
                        }
                        else
                        {
                            *(int *)value = SENSOR_AR0237;
						    sensorType =sensorid = SENSOR_AR0237;
						    printf(">>>>check sensor ID : AR0237MIPI<<<<\n");
						    return 0;
                        }
					}
					ret = reg_spi_read(0x208);
					printf("read 0x319 is 0x%x\n",ret);
					if(ret == 0xa0)
					{
						*(int *)value = SENSOR_IMX290;
						sensorType =sensorid = SENSOR_IMX290;
						printf(">>>>check sensor ID : AR0290<<<<\n");
						return 0;
					}
					ret = reg_spi_read(0x208);
					printf("ddddd----read 0x204 is 0x%x\n",ret);
					if(ret == 0x01)
					{
						*(int *)value = SENSOR_IMX123;
						sensorType =sensorid = SENSOR_IMX123;
						printf(">>>>check sensor ID : IMX123<<<<\n");
						return 0;
					}
					ret = reg_spi_read(0x208);
					if(ret==0x10)
					{
						*(int *)value = SENSOR_IMX185;
						sensorType =sensorid = SENSOR_IMX185;
						printf(">>>>check sensor ID : IMX185<<<\n");
						return 0;

					}
                    *(int *)value = SENSOR_IMX178;
                    sensorType = sensorid = SENSOR_IMX178;
                    printf(">>>>check sensor ID : imx178<<<<\n");
                 			
                    break;
                }
        
		default:
			break;
	}
	return 0;
}


int __base_isp_set_fps(float frameRate)
{
	if(frameRate>=2)
	{
		ISP_PUB_ATTR_S stPubAttr;
		HI_MPI_ISP_GetPubAttr(0, &stPubAttr);	
		stPubAttr.f32FrameRate = frameRate;
		HI_MPI_ISP_SetPubAttr(0, &stPubAttr);
		CURRENT_SENSOR_FPS =stPubAttr.f32FrameRate;
		printf("now sensor fps  %f\n",CURRENT_SENSOR_FPS);
	}
	return 0;
}

float JV_ISP_COMM_Get_Fps()
{

	return CURRENT_SENSOR_FPS;
}


BOOL JV_ISP_COMM_Query_LowFps()
{
	BOOL bLow = (LowFpsRatio == 16 ? FALSE:TRUE);
	return bLow;
}


int JV_ISP_COMM_Set_LowFps(int ratio)
{
	if (ratio<=0)
		return 0;
	
	pthread_mutex_lock(&isp_rf_mutex);
	
	float nFps= (float)USER_SENSOR_FPS*16/ratio;//使用
	nFps = ((float)((int)((nFps+0.05)*10)))/10;//保留小数点1位
	 __base_isp_set_fps(nFps);
	LowFpsRatio = ratio;
	pthread_mutex_unlock(&isp_rf_mutex);
	return 0;
}


int JV_ISP_COMM_Set_StdFps(int fps)
{

	pthread_mutex_lock(&isp_rf_mutex);
	
	float nFps= (float)fps*16/LowFpsRatio;
	nFps = ((float)((int)((nFps+0.05)*10)))/10;//保留小数点1位
	 __base_isp_set_fps(nFps);
	 
	USER_SENSOR_FPS = fps;
	
	pthread_mutex_unlock(&isp_rf_mutex);
	return 0;
}

int JV_ISP_COMM_Get_StdFps(void) //用户设置的sensor基准帧频 
{

	return USER_SENSOR_FPS;
}





int isp_exposure_set(EXPOSURE_DATA * pdata)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	//stExpAttr.enOpType = pdata->type;
	//stExpAttr.stManual.enExpTimeOpType = pdata->bManualExpLineEnable;
	//stExpAttr.stManual.u32ExpTime = pdata->u32ExpLine;
	//stExpAttr.stManual.enAGainOpType = pdata->bManualAGainEnable;
	//stExpAttr.stManual.u32AGain = pdata->s32AGain;
	//stExpAttr.stManual.enDGainOpType = pdata->bManualDGainEnable;
	//stExpAttr.stManual.u32DGain = pdata->s32DGain;
	//stExpAttr.stManual.enISPDGainOpType = pdata->bManualISPDGainEnable;
	//stExpAttr.stManual.u32ISPDGain = pdata->u32ISPDGain;
	//stExpAttr.stAuto.enAEMode = pdata->enAEMode;
	//stExpAttr.stAuto.u8Tolerance = pdata->s16ExpTolerance;
	//stExpAttr.stAuto.stAGainRange.u32Max = pdata->u16AGainMax;
	//stExpAttr.stAuto.stAGainRange.u32Min = pdata->u16AGainMin;
	//stExpAttr.stAuto.stDGainRange.u32Max = pdata->u16DGainMax;
	//stExpAttr.stAuto.stDGainRange.u32Min = pdata->u16DGainMin;
	//stExpAttr.stAuto.stExpTimeRange.u32Max = pdata->u16ExpTimeMax;
	//stExpAttr.stAuto.stExpTimeRange.u32Min = pdata->u16ExpTimeMin;
	//stExpAttr.stAuto.u8Compensation = pdata->u8ExpCompensation;
	//stExpAttr.stAuto.u8Speed = pdata->u8ExpStep;
	memcpy(&(stExpAttr.stAuto.au8Weight),&(pdata->u8Weight),AE_ZONE_ROW*AE_ZONE_COLUMN);
	CHECK_RET(HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr));

	return 0;
}

int isp_exposure_get(EXPOSURE_DATA * pdata)
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;

	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	pdata->type = stExpAttr.enOpType;
	pdata->s32AGain = stExpAttr.stManual.u32AGain;
	pdata->s32DGain = stExpAttr.stManual.u32DGain;
	pdata->u32ExpLine = stExpAttr.stManual.u32ExpTime;
	pdata->u32ISPDGain = stExpAttr.stManual.u32ISPDGain;
	pdata->bManualExpLineEnable = stExpAttr.stManual.enExpTimeOpType;
	pdata->bManualAGainEnable = stExpAttr.stManual.enAGainOpType;
	pdata->bManualDGainEnable = stExpAttr.stManual.enDGainOpType;
	pdata->bManualISPDGainEnable = stExpAttr.stManual.enISPDGainOpType;
	pdata->enAEMode = stExpAttr.stAuto.enAEMode;
	pdata->u16ExpTimeMax = stExpAttr.stAuto.stExpTimeRange.u32Max;
	pdata->u16ExpTimeMin = stExpAttr.stAuto.stExpTimeRange.u32Min;
	pdata->u16DGainMax = stExpAttr.stAuto.stDGainRange.u32Max;
	pdata->u16DGainMin = stExpAttr.stAuto.stDGainRange.u32Min;
	pdata->u16AGainMax = stExpAttr.stAuto.stAGainRange.u32Max;
	pdata->u16AGainMin = stExpAttr.stAuto.stAGainRange.u32Min;
	pdata->u8ExpStep = stExpAttr.stAuto.u8Speed;
	pdata->s16ExpTolerance = stExpAttr.stAuto.u8Tolerance;
	pdata->u8ExpCompensation = stExpAttr.stAuto.u8Compensation;
	memcpy(&(pdata->u8Weight),&(stExpAttr.stAuto.au8Weight),AE_ZONE_ROW*AE_ZONE_COLUMN);

	return 0;
}



int isp_wb_set(WB_DATA *pdata)
{
	ISP_DEV IspDev = 0;
	ISP_WB_ATTR_S stWBAttr;

	HI_MPI_ISP_GetWBAttr(IspDev, &stWBAttr);
	stWBAttr.enOpType = pdata->type;
	stWBAttr.stManual.u16Bgain = pdata->u16Bgain;
	//stWBAttr.stManual.u16Grgain = pdata->u16Ggain;
	//stWBAttr.stManual.u16Gbgain = pdata->u16Ggain;
	stWBAttr.stManual.u16Rgain = pdata->u16Rgain;
	stWBAttr.stAuto.bEnable = HI_TRUE;
	stWBAttr.stAuto.u8BGStrength = pdata->u8BGStrength;
	stWBAttr.stAuto.u8RGStrength = pdata->u8RGStrength;    
	stWBAttr.stAuto.u16ZoneSel = pdata->u8ZoneSel;
	stWBAttr.stAuto.u16HighColorTemp = pdata->u8HighColorTemp;
	stWBAttr.stAuto.u16LowColorTemp = pdata->u8LowColorTemp;
	HI_MPI_ISP_SetWBAttr(IspDev, &stWBAttr);

	return 0;
}

int isp_wb_get(WB_DATA * pdata)
{
	ISP_DEV IspDev = 0;
	ISP_WB_ATTR_S stWBAttr;

	HI_MPI_ISP_GetWBAttr(IspDev, &stWBAttr);
	pdata->type = stWBAttr.enOpType;
	pdata->u16Bgain = stWBAttr.stManual.u16Bgain;
	//pdata->u16Ggain = stWBAttr.stManual.u16Grgain;
	//pdata->u16Ggain = stWBAttr.stManual.u16Gbgain;
	pdata->u16Rgain = stWBAttr.stManual.u16Rgain;
	pdata->u8BGStrength = stWBAttr.stAuto.u8BGStrength;
	pdata->u8RGStrength = stWBAttr.stAuto.u8RGStrength;    
	pdata->u8ZoneSel = stWBAttr.stAuto.u16ZoneSel;
	pdata->u8HighColorTemp = stWBAttr.stAuto.u16HighColorTemp;
	pdata->u8LowColorTemp = stWBAttr.stAuto.u16LowColorTemp;

	return 0;
}



int isp_gamma_set(GAMMA_DATA *pdata)
{
	ISP_DEV IspDev = 0;
	ISP_GAMMA_ATTR_S stGammaAttr;

	stGammaAttr.bEnable = pdata->bEnable.bEnable;
	stGammaAttr.enCurveType = pdata->bEnable.enCurveType;
	memcpy(&stGammaAttr.u16Table,&pdata->u16Gamma,257 * 2);
	HI_MPI_ISP_SetGammaAttr(IspDev, &stGammaAttr);

	return 0;
}

int isp_gamma_get(GAMMA_DATA *pdata)
{
	ISP_DEV IspDev = 0;
	ISP_GAMMA_ATTR_S stGammaAttr;

	HI_MPI_ISP_GetGammaAttr(IspDev, &stGammaAttr);
	pdata->bEnable.bEnable = stGammaAttr.bEnable;
	pdata->bEnable.enCurveType = stGammaAttr.enCurveType;
	pdata->enGammaCurve = stGammaAttr.enCurveType;
	memcpy(&(pdata->u16Gamma),&(stGammaAttr.u16Table),257 * 2);

	return 0;
}




int isp_sharpen_set(SHARPEN_DATA *pdata)
{
	ISP_DEV IspDev = 0;
	ISP_SHARPEN_ATTR_S pstSharpenAttr;

	pstSharpenAttr.bEnable = pdata->bEnable;
	pstSharpenAttr.enOpType = pdata->bManualEnable;
	// ......
	HI_MPI_ISP_SetSharpenAttr(IspDev, &pstSharpenAttr);

	return 0;
}

int isp_sharpen_get(SHARPEN_DATA *pdata)
{
	ISP_DEV IspDev = 0;
	ISP_SHARPEN_ATTR_S pstSharpenAttr;

	HI_MPI_ISP_GetSharpenAttr(IspDev, &pstSharpenAttr);
	pdata->bEnable = pstSharpenAttr.bEnable;
	pdata->bManualEnable = pstSharpenAttr.enOpType;
	// ......

    return 0;
}



int isp_ccm_set(CCM_DATA *pdata)
{
	ISP_DEV IspDev = 0;

	ISP_SATURATION_ATTR_S pstSatAttr;
	pstSatAttr.enOpType = pdata->bSatManualEnable;
	pstSatAttr.stManual.u8Saturation = pdata->u8SatTarget;
	memcpy(pstSatAttr.stAuto.au8Sat, pdata->au8Sat, 16);
	HI_MPI_ISP_SetSaturationAttr(IspDev, &pstSatAttr);

	//HI_U32 pu32Value;
	//pu32Value = pdata->pu32Value;
	//HI_MPI_ISP_SetSaturation(pu32Value);

	ISP_COLORMATRIX_ATTR_S pstCCMAttr;
	HI_MPI_ISP_GetCCMAttr(IspDev, &pstCCMAttr);
	pstCCMAttr.stAuto.u16HighColorTemp = pdata->u16HighColorTemp;
	pstCCMAttr.stAuto.u16MidColorTemp = pdata->u16MidColorTemp;
	pstCCMAttr.stAuto.u16LowColorTemp = pdata->u16LowColorTemp;
	memcpy(&(pstCCMAttr.stAuto.au16HighCCM),&(pdata->au16HighCCM),9*2);
	memcpy(&(pstCCMAttr.stAuto.au16MidCCM),&(pdata->au16MidCCM),9*2);
	memcpy(&(pstCCMAttr.stAuto.au16LowCCM),&(pdata->au16LowCCM),9*2);
	HI_MPI_ISP_SetCCMAttr(IspDev, &pstCCMAttr);

	return 0;
}

int isp_ccm_get(CCM_DATA *pdata)
{
	ISP_DEV IspDev = 0;

	ISP_SATURATION_ATTR_S pstSatAttr;
	HI_MPI_ISP_GetSaturationAttr(IspDev, &pstSatAttr);
	pdata->bSatManualEnable = pstSatAttr.enOpType;
	pdata->u8SatTarget = pstSatAttr.stManual.u8Saturation;
	memcpy(pdata->au8Sat, pstSatAttr.stAuto.au8Sat, 16);

	//HI_U32 pu32Value;
	//HI_MPI_ISP_GetSaturation(&pu32Value);
	//pdata->pu32Value = pu32Value;

	ISP_COLORMATRIX_ATTR_S pstCCMAttr;
	HI_MPI_ISP_GetCCMAttr(IspDev, &pstCCMAttr);
	pdata->u16HighColorTemp = pstCCMAttr.stAuto.u16HighColorTemp;
	pdata->u16LowColorTemp = pstCCMAttr.stAuto.u16LowColorTemp;
	pdata->u16MidColorTemp = pstCCMAttr.stAuto.u16MidColorTemp;
	memcpy(&(pdata->au16HighCCM),&(pstCCMAttr.stAuto.au16HighCCM),9*2);
	memcpy(&(pdata->au16MidCCM),&(pstCCMAttr.stAuto.au16MidCCM),9*2);
	memcpy(&(pdata->au16LowCCM),&(pstCCMAttr.stAuto.au16LowCCM),9*2);

	return 0;
}


