#include <mme.h>

#include <linux/fs.h>
#include <linux/module.h>

#include "simple_test.h"

MODULE_DESCRIPTION("MME Local Transformer");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("Copyright 2004 STMicroelectronics, All rights reserved");

unsigned instantiatedCounter;

int init_module(void) {

  return MME_RegisterTransformer("com.st.mcdt.mme.local",
                                SimpleTest_AbortCommand,
                                SimpleTest_GetTransformerCapability,
                                SimpleTest_InitTransformer,
                                SimpleTest_ProcessCommand,
                                SimpleTest_TermTransformer);
        
}
 
void cleanup_module(void)
{
}

void harness_exit(void) {
}
