/*
 ============================================================================
 Name        : types_c.h
 Author      : Liuchen
 Version     :
 Copyright   : Your copyright notice
 Description : c types
 Created on	 : 2016-04-11
 ============================================================================
 */

#ifndef _MML_CORE_TYPES_H_
#define _MML_CORE_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <mml/core/memory.h>

#ifndef MML_INLINE
#  if defined __cplusplus
#    define MML_INLINE inline
#  elif defined _MSC_VER
#    define MML_INLINE __inline
#  else
#    define MML_INLINE static
#  endif
#endif /* MML_INLINE */

/* MMLPoint */
typedef struct _MMLPoint {
    int32_t x;
    int32_t y;
} MMLPoint;

MML_INLINE MMLPoint mmlPoint(int32_t x, int32_t y) {
    MMLPoint p;
    p.x = x;
    p.y = y;
    return p;
}

/* MMLSize */
typedef struct _MMLSize {
    int width;
    int height;
} MMLSize;

MML_INLINE MMLSize mmlSize(int width, int height) {
    MMLSize s;
    s.width = width;
    s.height = height;
    return s;
}

/* MMLRect */
typedef struct _MMLRect {
    int x;
    int y;
    int width;
    int height;
} MMLRect;

MML_INLINE MMLRect mmlRect(int x, int y, int width, int height) {
    MMLRect r;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    return r;
}

/* MMLImageConstraint */
typedef struct _MMLImageConstraint {
    int w_align;
    int h_align;
    int w_min;
    int h_min;
} MMLImageConstraint;

MML_INLINE MMLImageConstraint mmlImageConstraint(\
        int w_align, int h_align, int w_min, int h_min) {
    MMLImageConstraint c;
    c.w_align = w_align;
    c.h_align = h_align;
    c.w_min = w_min;
    c.h_min = h_min;
    return c;
}

/*
 * The following definitions
 * is mml image headers.
 */
#define MML_DEPTH_SIGN 0x80000000

#define MML_DEPTH_1U     1
#define MML_DEPTH_4U     4
#define MML_DEPTH_5U     5
#define MML_DEPTH_8U     8
#define MML_DEPTH_16U   16
#define MML_DEPTH_32F   32
#define MML_DEPTH_32U   32

#define MML_DEPTH_8S  (MML_DEPTH_SIGN| 8)
#define MML_DEPTH_16S (MML_DEPTH_SIGN|16)
#define MML_DEPTH_32S (MML_DEPTH_SIGN|32)

#define MML_ALIGN_2BYTES   2
#define MML_ALIGN_4BYTES   4
#define MML_ALIGN_8BYTES   8
#define MML_ALIGN_16BYTES 16
#define MML_ALIGN_32BYTES 32
#define MML_ALIGN_64BYTES 32

#define MML_ALIGN_DWORD   MML_ALIGN_4BYTES
#define MML_ALIGN_QWORD   MML_ALIGN_8BYTES

#ifdef _HISI_PLATFORM_
#define HASH_MML_IMAGE 0x13680b4b
#else
#define HASH_MML_IMAGE 0xfe6d251f
#endif

typedef struct _MMLImage {
    int  n_channels;         /**< Most of MML functions support 1,2,3 or 4 channels */
    int  depth;             /**< Pixel depth in bits: MML_DEPTH_8U, MML_DEPTH_8S, MML_DEPTH_16S,
                               MML_DEPTH_32S, MML_DEPTH_32F and MML_DEPTH_64F are supported.  */
    int  width;             /**< Image width in pixels. */
    int  width_origin;      /**< Image origin width in pixels. */
    int  height;            /**< Image height in pixels. */
    int  height_origin;     /**< Image origin height in pixels. */
    MMLRect roi;    /**< Image ROI. If NULL, the whole image is selected. */
    MMLRect roi_origin;    /**< Image Origin ROI. If NULL, the whole image is selected. */
    int  image_size;         /**< Image data size in bytes
                               (==image->height*image->widthStep
                               in case of interleaved data)*/
    unsigned char *image_data;        /**< Pointer to aligned image data. */
#ifdef _HISI_PLATFORM_
    unsigned int image_data_phy;        /**< Pointer to aligned image data. */
#endif
    int  image_size_origin;         /**< very origin image data size */
    unsigned char *image_data_origin;  /**< Pointer to very origin of image data */
#ifdef _HISI_PLATFORM_
    unsigned int image_data_origin_phy;  /**< Pointer to very origin of image data */
#endif
    unsigned int self_check;
} MMLImage;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlCreateImageHeader create mml image header
 * @param img [in] image header pointer
 * @param size [in] image size
 * @param depth [in] image depth
 * @param channels [in] image channels
 * @param cond [in] image constraint condition
 * @return img mmlimage header
 */
MMLImage * mmlInitImageHeader(MMLImage * img, \
        MMLSize size, int depth, int channels, \
        MMLImageConstraint cond);

/**
 * mmlSizeAlign Align mml image size
 * @param src [in] unaligned size
 * @param cond [in] constraint condition
 * @return dst aligned size
 */
MMLSize mmlSizeAlign(const MMLSize src, MMLImageConstraint cond);

/**
 * mmlSizeMin amend mml image origin size bigger | equal than min size
 * @param src [in] origin size
 * @param cond [in] constraint condition
 * @return dst size bigger | equal than min size
 */
MMLSize mmlSizeMin(const MMLSize src, MMLImageConstraint cond);

/**
 * mmlCreateImageHeader create mml image header
 * @param size [in] image size
 * @param depth [in] image depth
 * @param channels [in] image channels
 * @return img mmlimage header
 */
MMLImage * mmlCreateImageHeader(MMLSize size, int depth, int channels);

/**
 * mmlReleaseImageHeader release mml image header
 * @param img [in] image pointer pointer
 * @return ret status
 */
int mmlReleaseImageHeader(MMLImage ** img);

/**
 * mmlImageSelfCheck check data structure is MMLImage or not
 * @param img [in] MMLImage which need check
 * @return 0 is MMLImage
 * @return -1 is not MMLImage
 */
int mmlImageSelfCheck(const MMLImage * img);

/**
 * mmlSetData set data for mml image
 * @param img [in] mml image pointer
 * @param data [in] image data pointer
 * @param step [in] image data stride
 * @return ret status
 */
int mmlSetData(MMLImage * img, void * data, int step);

#ifdef _HISI_PLATFORM_
/**
 * mmlHisiSetData set data for mml image
 * @param img [in] mml image pointer
 * @param data [in] image data pointer
 * @param step [in] image data stride
 * @return ret status
 */
int mmlHisiSetData(MMLImage * img, MMLMMZ data, int step);
#endif /* _HISI_PLATFORM_ */

/**
 * mmlCreateImage create mml image
 * @param size [in] image size
 * @param depth [in] image depth
 * @param channels [in] image channels
 * @return img mmlimage
 */
MMLImage * mmlCreateImage(MMLSize size, int depth, int channels);

/**
 * mmlReleaseImage release mml image header
 * @param img [in] image pointer pointer
 * @return ret status
 */
int mmlReleaseImage(MMLImage ** img);

/**
 * mmlShapeImage Change the shape of Image
 * @param img [in] image pointer pointer
 * @param size [in] new image shape
 * @param realloc [in] Alloc the new space or not when the memory is not enough
 *                      false no; true yes.
 * @return ret status
 */
int mmlShapeImage(MMLImage * img, MMLSize new_size, bool realloc);

#ifdef _HISI_PLATFORM_
#define HASH_MML_MEM_STORAGE 0x666a15ce
#else
#define HASH_MML_MEM_STORAGE 0x4d3b5359
#endif

// mml memory storage
typedef struct _MMLMemStorage{
    int mem_size; // memory storage
    unsigned char *mem_data; // memory storage data pointer
#ifdef _HISI_PLATFORM_
    unsigned int mem_data_phy; // memory storage data physic address
#endif
    unsigned int self_check; // self check hash number
} MMLMemStorage;

/**
 * mmlCreateMemStorage create mml memory storage
 * @param size [in] memory storage size
 * @return mem memory storage
 */
MMLMemStorage * mmlCreateMemStorage(int size);

/**
 * mmlReleaseImage release mml memory storage
 * @param img [in] mml memory storage pointer pointer
 * @return ret status
 */
int mmlReleaseMemStorage(MMLMemStorage ** mem);

/**
 * mmlSizeAlign Align mml memory storage size
 * @param size [in] unaligned size
 * @param align [in] align condition
 * @return dst aligned memory storage size
 */
int mmlMemSizeAlign(int src, int align);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_CORE_TYPES_H_ */
