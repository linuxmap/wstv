/*
 ============================================================================
 Name        : seqgen.h
 Author      : Liuchen
 Version     :
 Copyright   : Your copyright notice
 Description : seqgen
 Created on	 : 2016-06-12
 ============================================================================
 */

#ifndef _MML_TOOL_SEQ_GEN_H_
#define _MML_TOOL_SEQ_GEN_H_

typedef struct _MMLSeqGenHandle {
    int seq_now;
} MMLSeqGenHandle;

typedef struct _MMLNameSeqGenHandle {
    MMLSeqGenHandle * sg;
    char * prefix;
    char * suffix;
    char * name;
} MMLNameSeqGenHandle;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlSeqGenInit Initialize Seq Generator
 * @return MMLSeqGenHandle * mml Sequence Generator handle
 */
MMLSeqGenHandle * mmlSeqGenInit(void);

/**
 * mmlSeqGenFinal Finalise Seq Generator
 * @param sg [in] Seq Generator pointer pointer
 * @return ret result of SeqGenFinal
 */
int mmlSeqGenFinal(MMLSeqGenHandle ** sg);

/**
 * mmlSeqGen Generate the seq number
 * @return seq number
 */
int mmlSeqGen(MMLSeqGenHandle * sg);

/**
 * mmlNameSeqGenInit Initialize NameSeq Generator
 * @return MMLNameSeqGenHandle * mml NameSequence Generator handle
 */
MMLNameSeqGenHandle * mmlNameSeqGenInit( \
        const char * prefix, const char * suffix);

/**
 * mmlNameSeqGenFinal Finalise NameSeq Generator
 * @param nsg [in] Name Seq Generator pointer pointer
 * @return ret result of NameSeqGenFinal
 */
int mmlNameSeqGenFinal(MMLNameSeqGenHandle ** nsg);

/**
 * mmlNameSeqGen Generate the seq number
 * @param nsg [in] Name Seq Generator pointer
 * @return name include sequence number
 */
char * mmlNameSeqGen(MMLNameSeqGenHandle * nsg);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_TOOL_SEQ_GEN_H_ */
