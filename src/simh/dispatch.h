/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: f32bf565-f629-11ec-8ceb-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2009-2019 Agner Fog
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------
 */

/* ######################################################################### */

#ifndef __DISPATCH_H_
# define __DISPATCH_H_

/* ######################################################################### */

# include <stdint.h>

/* ######################################################################### */

# if defined( __INTEL_COMPILER ) || defined( __INTEL_CLANG_COMPILER ) \
  || defined( __INTEL_LLVM_COMPILER ) || defined( INTEL_MKL_VERSION ) \
  || defined( __INTEL_MKL__ )

/* ######################################################################### */

  extern int64_t  __intel_cpu_feature_indicator;
  extern int64_t  __intel_cpu_feature_indicator_x;
  extern int64_t  __intel_mkl_feature_indicator;
  extern int64_t  __intel_mkl_feature_indicator_x;

/* ######################################################################### */

  void __intel_cpu_features_init();
  void __intel_cpu_features_init_x();

/* ######################################################################### */

#  if defined( INTEL_MKL_VERSION ) || defined( __INTEL_MKL__ )
    void __intel_mkl_features_init();
    void __intel_mkl_features_init_x();
#  endif /* if defined( INTEL_MKL_VERSION ) || defined( __INTEL_MKL__ ) */

/* ######################################################################### */

  void agner_compiler_patch();

/* ######################################################################### */

#  if defined( __INTEL_COMPILER ) || defined( __INTEL_CLANG_COMPILER ) \
   || defined( __INTEL_LLVM_COMPILER )
    void agner_cpu_patch();
#  endif /* if defined( __INTEL_COMPILER ) || defined( __INTEL_CLANG_COMPILER )
            || defined( __INTEL_LLVM_COMPILER ) */

/* ######################################################################### */

#  if defined( INTEL_MKL_VERSION ) || defined( __INTEL_MKL__ )
    void agner_mkl_patch();
#  endif /* if defined( INTEL_MKL_VERSION ) || defined( __INTEL_MKL__ ) */

/* ######################################################################### */

# endif /* if defined( __INTEL_COMPILER ) || defined( __INTEL_CLANG_COMPILER )
           || defined( __INTEL_LLVM_COMPILER ) || defined( INTEL_MKL_VERSION )
           || defined( __INTEL_MKL__ ) */

/* ######################################################################### */

#endif /* ifndef __DISPATCH_H_ */

/* ######################################################################### */
