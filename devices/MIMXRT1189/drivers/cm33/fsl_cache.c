/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_cache.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.cache_xcache"
#endif

#if (FSL_FEATURE_SOC_XCACHE_COUNT > 0)
/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Array of XCACHE peripheral base address. */
static XCACHE_Type *const s_xcachectrlBases[] = XCACHE_BASE_PTRS;

#if (defined(FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT) && (FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT > 0))
/* Array of XCACHE_POLSEL peripheral base address. */
static XCACHE_POLSEL_Type *const s_xcachepolselBases[] = XCACHE_POLSEL_BASE_PTRS;
#endif

/* Array of XCACHE physical memory base address. */
static uint32_t const s_xcachePhymemBases[] = XCACHE_PHYMEM_BASES;
/* Array of XCACHE physical memory size. */
static uint32_t const s_xcachePhymemSizes[] = XCACHE_PHYMEM_SIZES;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
#ifdef XCACHE_CLOCKS
/* Array of XCACHE clock name. */
static const clock_ip_name_t s_xcacheClocks[] = XCACHE_CLOCKS;
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (defined(FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT) && (FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT > 0))
/*!
 * brief Returns an instance number given periphearl base address.
 *
 * param base The peripheral base address.
 * return XCACHE_POLSEL instance number starting from 0.
 */
uint32_t XCACHE_GetInstance(XCACHE_POLSEL_Type *base)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(s_xcachepolselBases); i++)
    {
        if (base == s_xcachepolselBases[i])
        {
            break;
        }
    }

    assert(i < ARRAY_SIZE(s_xcachepolselBases));

    return i;
}
#endif

/*!
 * brief Returns an instance number given physical memory address.
 *
 * param address The physical memory address.
 * return XCACHE instance number starting from 0.
 */
uint32_t XCACHE_GetInstanceByAddr(uint32_t address)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(s_xcachectrlBases); i++)
    {
        if ((address >= s_xcachePhymemBases[i]) && (address < s_xcachePhymemBases[i] + s_xcachePhymemSizes[i]))
        {
            break;
        }
    }

    return i;
}

#if (defined(FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT) && (FSL_FEATURE_SOC_XCACHE_POLSEL_COUNT > 0))
/*!
 * @brief Initializes an XCACHE instance with the user configuration structure.
 *
 * This function configures the XCACHE module with user-defined settings. Call the XCACHE_GetDefaultConfig() function
 * to configure the configuration structure and get the default configuration.
 *
 * @param base XCACHE_POLSEL peripheral base address.
 * @param config Pointer to a user-defined configuration structure.
 * @retval kStatus_Success XCACHE initialize succeed
 */
status_t XCACHE_Init(XCACHE_POLSEL_Type *base, const xcache_config_t *config)
{
    volatile uint32_t *topReg = &base->REG0_TOP;
    uint32_t i;
    uint32_t polsel = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
#ifdef XCACHE_CLOCKS
    uint32_t instance = XCACHE_GetInstance(base);

    /* Enable XCACHE clock */
    CLOCK_EnableClock(s_xcacheClocks[instance]);
#endif
#endif

    for (i = 0; i < XCACHE_REGION_NUM - 1U; i++)
    {
        assert((config->boundaryAddr[i] & (XCACHE_REGION_ALIGNMENT - 1U)) == 0U);
        ((volatile uint32_t *)topReg)[i] =
            config->boundaryAddr[i] >= XCACHE_REGION_ALIGNMENT ? config->boundaryAddr[i] - XCACHE_REGION_ALIGNMENT : 0U;
    }

    for (i = 0; i < XCACHE_REGION_NUM; i++)
    {
        polsel |= (((uint32_t)config->policy[i]) << (2U * i));
    }
    base->POLSEL = polsel;

    return kStatus_Success;
}

/*!
 * @brief Gets the default configuration structure.
 *
 * This function initializes the XCACHE configuration structure to a default value. The default
 * values are first region covers whole cacheable area, and policy set to write back.
 *
 * @param config Pointer to a configuration structure.
 */
void XCACHE_GetDefaultConfig(xcache_config_t *config)
{
    (void)memset(config, 0, sizeof(xcache_config_t));

    config->boundaryAddr[0] = s_xcachePhymemSizes[0];
    config->policy[0]       = kXCACHE_PolicyWriteBack;
}
#endif

/*!
 * brief Enables the cache.
 *
 */
void XCACHE_EnableCache(XCACHE_Type *base)
{
    /* First, invalidate the entire cache. */
    XCACHE_InvalidateCache(base);

    /* Now enable the cache. */
    base->CCR |= XCACHE_CCR_ENCACHE_MASK;
}

/*!
 * brief Disables the cache.
 *
 */
void XCACHE_DisableCache(XCACHE_Type *base)
{
    /* First, push any modified contents. */
    XCACHE_CleanCache(base);

    /* Now disable the cache. */
    base->CCR &= ~XCACHE_CCR_ENCACHE_MASK;
}

/*!
 * brief Invalidates the cache.
 *
 */
void XCACHE_InvalidateCache(XCACHE_Type *base)
{
    /* Invalidate all lines in both ways and initiate the cache command. */
    base->CCR |= XCACHE_CCR_INVW0_MASK | XCACHE_CCR_INVW1_MASK | XCACHE_CCR_GO_MASK;

    /* Wait until the cache command completes. */
    while ((base->CCR & XCACHE_CCR_GO_MASK) != 0x00U)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->CCR &= ~(XCACHE_CCR_INVW0_MASK | XCACHE_CCR_INVW1_MASK);
}

/*!
 * brief Invalidates cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be invalidated.
 * note Address and size should be aligned to "L1CODCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to XCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void XCACHE_InvalidateCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg  = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~((uint32_t)XCACHE_LINESIZE_BYTE - 1U);
    uint32_t instance  = XCACHE_GetInstanceByAddr(address);
    uint32_t endLim;
    XCACHE_Type *base;

    if (instance >= ARRAY_SIZE(s_xcachectrlBases))
    {
        return;
    }
    base    = s_xcachectrlBases[instance];
    endLim  = s_xcachePhymemBases[instance] + s_xcachePhymemSizes[instance];
    endAddr = endAddr > endLim ? endLim : endAddr;

    /* Set the invalidate by line command and use the physical address. */
    pccReg     = (base->CLCR & ~XCACHE_CLCR_LCMD_MASK) | XCACHE_CLCR_LCMD(1) | XCACHE_CLCR_LADSEL_MASK;
    base->CLCR = pccReg;

    while (startAddr < endAddr)
    {
        /* Set the address and initiate the command. */
        base->CSAR = (startAddr & XCACHE_CSAR_PHYADDR_MASK) | XCACHE_CSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while ((base->CSAR & XCACHE_CSAR_LGO_MASK) != 0x00U)
        {
        }
        startAddr += (uint32_t)XCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans the cache.
 *
 */
void XCACHE_CleanCache(XCACHE_Type *base)
{
    /* Enable the to push all modified lines. */
    base->CCR |= XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK | XCACHE_CCR_GO_MASK;

    /* Wait until the cache command completes. */
    while ((base->CCR & XCACHE_CCR_GO_MASK) != 0x00U)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->CCR &= ~(XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK);
}

/*!
 * brief Cleans cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be cleaned.
 * note Address and size should be aligned to "XCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to XCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void XCACHE_CleanCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg  = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~((uint32_t)XCACHE_LINESIZE_BYTE - 1U);
    uint32_t instance  = XCACHE_GetInstanceByAddr(address);
    uint32_t endLim;
    XCACHE_Type *base;

    if (instance >= ARRAY_SIZE(s_xcachectrlBases))
    {
        return;
    }
    base    = s_xcachectrlBases[instance];
    endLim  = s_xcachePhymemBases[instance] + s_xcachePhymemSizes[instance];
    endAddr = endAddr > endLim ? endLim : endAddr;

    /* Set the push by line command. */
    pccReg     = (base->CLCR & ~XCACHE_CLCR_LCMD_MASK) | XCACHE_CLCR_LCMD(2) | XCACHE_CLCR_LADSEL_MASK;
    base->CLCR = pccReg;

    while (startAddr < endAddr)
    {
        /* Set the address and initiate the command. */
        base->CSAR = (startAddr & XCACHE_CSAR_PHYADDR_MASK) | XCACHE_CSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while ((base->CSAR & XCACHE_CSAR_LGO_MASK) != 0x00U)
        {
        }
        startAddr += (uint32_t)XCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans and invalidates the cache.
 *
 */
void XCACHE_CleanInvalidateCache(XCACHE_Type *base)
{
    /* Push and invalidate all. */
    base->CCR |= XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK | XCACHE_CCR_INVW0_MASK | XCACHE_CCR_INVW1_MASK |
                 XCACHE_CCR_GO_MASK;

    /* Wait until the cache command completes. */
    while ((base->CCR & XCACHE_CCR_GO_MASK) != 0x00U)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->CCR &= ~(XCACHE_CCR_PUSHW0_MASK | XCACHE_CCR_PUSHW1_MASK | XCACHE_CCR_INVW0_MASK | XCACHE_CCR_INVW1_MASK);
}

/*!
 * brief Cleans and invalidate cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be Cleaned and Invalidated.
 * note Address and size should be aligned to "XCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to XCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void XCACHE_CleanInvalidateCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg  = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~((uint32_t)XCACHE_LINESIZE_BYTE - 1U);
    uint32_t instance  = XCACHE_GetInstanceByAddr(address);
    uint32_t endLim;
    XCACHE_Type *base;

    if (instance >= ARRAY_SIZE(s_xcachectrlBases))
    {
        return;
    }
    base    = s_xcachectrlBases[instance];
    endLim  = s_xcachePhymemBases[instance] + s_xcachePhymemSizes[instance];
    endAddr = endAddr > endLim ? endLim : endAddr;

    /* Set the push by line command. */
    pccReg     = (base->CLCR & ~XCACHE_CLCR_LCMD_MASK) | XCACHE_CLCR_LCMD(3) | XCACHE_CLCR_LADSEL_MASK;
    base->CLCR = pccReg;

    while (startAddr < endAddr)
    {
        /* Set the address and initiate the command. */
        base->CSAR = (startAddr & XCACHE_CSAR_PHYADDR_MASK) | XCACHE_CSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while ((base->CSAR & XCACHE_CSAR_LGO_MASK) != 0x00U)
        {
        }
        startAddr += (uint32_t)XCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Enable the cache write buffer.
 *
 */
void XCACHE_EnableWriteBuffer(XCACHE_Type *base, bool enable)
{
    if (enable)
    {
        base->CCR |= XCACHE_CCR_ENWRBUF_MASK;
    }
    else
    {
        base->CCR &= ~XCACHE_CCR_ENWRBUF_MASK;
    }
}

#endif /* FSL_FEATURE_SOC_XCACHE_COUNT > 0 */
