//<<<<<<< rnngpu
/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2017 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#include <miopen/rnn.hpp>
#include <miopen/env.hpp>
#include <miopen/errors.hpp>

// MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_AMD_ROCM_PRECOMPILED_BINARIES)
// MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_AMD_ASM_KERNELS_PERF_FILTERING)

namespace miopen {

RNNDescriptor::RNNDescriptor(int p_seqLength, int p_layer, int p_bidir, int p_bias)
    : mode(miopenRNNTANH), seqLength(p_seqLength), layer(p_layer), bidir(p_bidir), bias(p_bias)
{
    if(seqLength < 0 || layer < 0)
    {
        MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN cannot be negative");
    }
	if (bidir != 0 && bidir != 1)
	{
		MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN directional type not supported");
	}
	if (bias != 0 && bias != 1)
	{
		MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN bias type not supported");
	}
}

RNNDescriptor::RNNDescriptor(miopenRNNMode_t p_mode, int p_seqLength, int p_layer, int p_bidir, int p_bias)
    : mode(p_mode), seqLength(p_seqLength), layer(p_layer), bidir(p_bidir), bias(p_bias)
{
    if(seqLength < 0 || layer < 0)
    {
        MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN cannot be negative");
    }
    if(!(mode == miopenRNNRELU || mode == miopenRNNTANH || mode == miopenLSTM || mode == miopenGRU))
    {
        MIOPEN_THROW(miopenStatusBadParm, "RNN mode not supported");
    }
	if (bidir != 0 && bidir != 1)
	{
		MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN directional type not supported");
	}
	if (bias != 0 && bias != 1)
	{
		MIOPEN_THROW(miopenStatusBadParm, "Parameters to RNN bias type not supported");
	}
}

/*

std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>
RNNDescriptor::GetForwardOutputDim(const TensorDescriptor& inputTensorDesc,
                                           const TensorDescriptor& filterDesc) const
{
    assert(inputTensorDesc.GetLengths().size() == 4);
    assert(filterDesc.GetLengths().size() == 4);

    if(inputTensorDesc.GetType() != filterDesc.GetType())
    {
        MIOPEN_THROW(miopenStatusBadParm, "Types do not match for the filter");
    }

    std::size_t input_n;
    std::size_t input_c;
    std::size_t input_h;
    std::size_t input_w;

    std::tie(input_n, input_c, input_h, input_w) = miopen::tie4(inputTensorDesc.GetLengths());

    std::size_t filter_k;
    std::size_t filter_c;
    std::size_t filter_h;
    std::size_t filter_w;

    std::tie(filter_k, filter_c, filter_h, filter_w) = miopen::tie4(filterDesc.GetLengths());

    if(mode == miopenConvolution)
    {
        if(input_c != filter_c)
        {
            MIOPEN_THROW(miopenStatusBadParm, "Channels do not match for the filter");
        }
    }
    else if(mode == miopenTranspose)
    {
        if(input_c != filter_k)
        {
            MIOPEN_THROW(miopenStatusBadParm, "Channels do not match for the filter");
        }
    }

    std::size_t output_c;
    std::size_t output_h;
    std::size_t output_w;
    if(mode == miopenTranspose)
    {
        output_c = filter_c;
        output_h = std::max<std::size_t>(
            1, u * (input_h - 1) + 1 + dilation_h * (filter_h - 1) - 2 * pad_h);
        output_w = std::max<std::size_t>(
            1, v * (input_w - 1) + 1 + dilation_w * (filter_w - 1) - 2 * pad_w);
    }
    else
    {
        output_c = filter_k;
        output_h = std::max<std::size_t>(
            1, (input_h - (1 + dilation_h * (filter_h - 1)) + 2 * pad_h) / u + 1);
        output_w = std::max<std::size_t>(
            1, (input_w - (1 + dilation_w * (filter_w - 1)) + 2 * pad_w) / v + 1);
    }

    return std::make_tuple(input_n, output_c, output_h, output_w);

    //	return std::make_tuple(
    //		input_n,
    //		filter_k,
    //		std::max(1, (input_h - filter_h + 2*pad_h) / u + 1),
    //		std::max(1, (input_w - filter_w + 2*pad_w) / v + 1)
    //	);
}

size_t RNNDescriptor::ForwardGetWorkSpaceSizeGEMM(Handle& handle,
                                                          const TensorDescriptor& wDesc,
                                                          const TensorDescriptor& yDesc) const
{
    int out_h, out_w;
    std::tie(std::ignore, std::ignore, out_h, out_w) = miopen::tie4(yDesc.GetLengths());

    int wei_c, wei_h, wei_w;
    std::tie(std::ignore, wei_c, wei_h, wei_w) = miopen::tie4(wDesc.GetLengths());

    size_t workspace_size = wei_c * wei_h * wei_w * out_h * out_w * sizeof(yDesc.GetType());

    // gfx803 devices have 4gb-6gb memory
    if(workspace_size > (1 << 30) && handle.GetDeviceName() == "gfx803")
    {
        workspace_size = 0;
    }

    return (wei_h == 1 && wei_w == 1 && v == 1 && u == 1) ? 0 : workspace_size;
}

// FIXME: This seems to duplicate
// mlo_construct_direct2D::mloIsCorrectBinaryWinograd3x3U()
// functionality thus violating the One Definition Rule.
bool RNNDescriptor::IsWinograd3x3Supported(Handle& handle,
                                                   bool direction,
                                                   const TensorDescriptor& wDesc,
                                                   const TensorDescriptor& xDesc) const
{
    const auto perf_filtering = miopen::IsEnabled(MIOPEN_DEBUG_AMD_ASM_KERNELS_PERF_FILTERING{});
    if(perf_filtering || miopen::IsDisabled(MIOPEN_DEBUG_AMD_ROCM_PRECOMPILED_BINARIES{}))
    {
        return false;
    }

    const auto device_name       = handle.GetDeviceName();
    const auto max_compute_units = handle.GetMaxComputeUnits();

    int _batch_sz, _n_inputs, _in_height, _in_width;
    int _n_outputs, _kernel_size0, _kernel_size1;
    int _n_outputs_w, _n_inputs_w;

    const auto device_is_gfx9_no_xnack = (device_name == "gfx900");
    const bool device_is_gfx8_no_xnack = (device_name == "gfx800" || device_name == "gfx802" ||
                                          device_name == "gfx803" || device_name == "gfx804");
    if(!device_is_gfx8_no_xnack && !device_is_gfx9_no_xnack)
    {
        return false;
    }

    std::tie(_batch_sz, _n_inputs, _in_height, _in_width)             = tie4(xDesc.GetLengths());
    std::tie(_n_outputs_w, _n_inputs_w, _kernel_size0, _kernel_size1) = tie4(wDesc.GetLengths());

    _n_outputs = direction ? _n_outputs_w : _n_inputs_w;
    return pad_h == 1 && pad_w == 1 && _kernel_size0 == 3 && _kernel_size1 == 3 && u == 1 &&
           v == 1 && _batch_sz < std::pow(2, 16) && _n_inputs < std::pow(2, 16) &&
           _n_outputs < std::pow(2, 16) && _in_height < std::pow(2, 16) &&
           _in_width < std::pow(2, 16) && max_compute_units < std::pow(2, 16) &&
           (_n_inputs * _in_height * _in_width) <= std::pow(2, 28) &&
           (_n_outputs * _in_height * _in_width) <= std::pow(2, 28) &&
           (_n_inputs * _kernel_size0 * _kernel_size1) <= std::pow(2, 28) &&
           (_n_outputs * _kernel_size0 * _kernel_size1) <= std::pow(2, 28) && _n_inputs % 2 == 0 &&
           _n_inputs >= (device_is_gfx8_no_xnack ? 16 : 18);
}

bool RNNDescriptor::IsBwdWeightsDirectSupported(const TensorDescriptor& wDesc) const
{
    int _kernel_size0, _kernel_size1;
    std::tie(std::ignore, std::ignore, _kernel_size0, _kernel_size1) = tie4(wDesc.GetLengths());

    return !(_kernel_size0 == 1 && _kernel_size1 == 1 && (u != 1 || v != 1));
}

bool RNNDescriptor::IsDirectSupported(const TensorDescriptor& wDesc) const
{
    int _kernel_size0, _kernel_size1;
    std::tie(std::ignore, std::ignore, _kernel_size0, _kernel_size1) = tie4(wDesc.GetLengths());

    return !(_kernel_size0 == 3 && _kernel_size1 == 3 && (pad_h > 1 || pad_w > 1));
}

size_t RNNDescriptor::ForwardGetWorkSpaceSize(Handle& handle,
                                                      const TensorDescriptor& wDesc,
                                                      const TensorDescriptor& xDesc,
                                                      const TensorDescriptor& yDesc) const
{
    if(mode == miopenTranspose)
        return BackwardDataGetWorkSpaceSizeGEMM(handle, wDesc, xDesc);
    else
    {
        if(dilation_w > 1 || dilation_h > 1)
            return ForwardGetWorkSpaceSizeGEMM(handle, wDesc, yDesc);

        // Check if Winograd is available
        // If Winograd is present, there is no advantage in letting
        // the user run another algorithm as those both slower and
        // use more workspace.
        if(IsWinograd3x3Supported(handle, true, wDesc, xDesc))
        {
            return 0;
        }
        else
        {
            size_t workspace_size_gemm = ForwardGetWorkSpaceSizeGEMM(handle, wDesc, yDesc);
            size_t workspace_size_fft  = ForwardGetWorkSpaceSizeFFT(wDesc, xDesc, yDesc);

            return (workspace_size_fft > workspace_size_gemm ? workspace_size_fft
                                                             : workspace_size_gemm);
        }
    }
}

size_t RNNDescriptor::BackwardDataGetWorkSpaceSize(Handle& handle,
                                                           const TensorDescriptor& wDesc,
                                                           const TensorDescriptor& dyDesc,
                                                           const TensorDescriptor& dxDesc) const
{
    if(mode == miopenTranspose)
        return ForwardGetWorkSpaceSizeGEMM(handle, wDesc, dxDesc);
    else
    {
        if(dilation_w > 1 || dilation_h > 1)
            return BackwardDataGetWorkSpaceSizeGEMM(handle, wDesc, dyDesc);

        // Check if Winograd is available
        // If Winograd is present, there is no advantage in letting
        // the user run another algorithm as those both slower and
        // use more workspace.
        if(IsWinograd3x3Supported(handle, false, wDesc, dyDesc))
        {
            return 0;
        }
        else
        {
            size_t workspace_size_gemm = BackwardDataGetWorkSpaceSizeGEMM(handle, wDesc, dyDesc);
            size_t workspace_size_fft  = BackwardGetWorkSpaceSizeFFT(wDesc, dyDesc, dxDesc);

            return (workspace_size_fft > workspace_size_gemm ? workspace_size_fft
                                                             : workspace_size_gemm);
        }
    }
}

// weights_n = output_c
// weights_c = input_c
// weights_h = 2*pad_h + input_h - u*(output_h - 1)
// weights_w = 2*pad_w + input_w - v*(output_w - 1)
std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>
RNNDescriptor::GetBackwardsWeightsDim(const TensorDescriptor& inputTensorDesc,
                                              const TensorDescriptor& outputTensorDesc) const
{
    assert(inputTensorDesc.GetLengths().size() == 4);
    assert(outputTensorDesc.GetLengths().size() == 4);

    if(inputTensorDesc.GetType() != outputTensorDesc.GetType())
    {
        MIOPEN_THROW(miopenStatusBadParm, "Types do not match for the filter");
    }

    std::size_t input_n;
    std::size_t input_c;
    std::size_t input_h;
    std::size_t input_w;

    std::tie(input_n, input_c, input_h, input_w) = miopen::tie4(inputTensorDesc.GetLengths());

    std::size_t output_n;
    std::size_t output_c;
    std::size_t output_h;
    std::size_t output_w;

    std::tie(output_n, output_c, output_h, output_w) = miopen::tie4(outputTensorDesc.GetLengths());

    // if(input_c != filter_c) {
    // 	MIOPEN_THROW(miopenStatusBadParm, "Channels do not match for the filter");
    // }

    return std::make_tuple(output_c,
                           input_c,
                           2 * pad_h + input_h - u * (output_h - 1),
                           2 * pad_w + input_w - v * (output_w - 1));
}

std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>
RNNDescriptor::GetBackwardOutputDim(const TensorDescriptor& outputTensorDesc,
                                            const TensorDescriptor& filterDesc) const
{
    assert(outputTensorDesc.GetLengths().size() == 4);
    assert(filterDesc.GetLengths().size() == 4);

    if(outputTensorDesc.GetType() != filterDesc.GetType())
    {
        MIOPEN_THROW(miopenStatusBadParm, "Types do not match for the filter");
    }

    std::size_t output_n;
    std::size_t output_c;
    std::size_t output_h;
    std::size_t output_w;

    std::tie(output_n, output_c, output_h, output_w) = miopen::tie4(outputTensorDesc.GetLengths());

    std::size_t filter_k;
    std::size_t filter_c;
    std::size_t filter_h;
    std::size_t filter_w;

    std::tie(filter_k, filter_c, filter_h, filter_w) = miopen::tie4(filterDesc.GetLengths());

    if(output_c != filter_k)
    {
        MIOPEN_THROW(miopenStatusBadParm, "Channels do not match for the filter");
    }

    return std::make_tuple(output_n,
                           filter_c,
                           u * (output_h - 1) - 2 * pad_h + filter_h,
                           v * (output_w - 1) - 2 * pad_w + filter_w);
}

TensorDescriptor
RNNDescriptor::GetForwardOutputTensor(const TensorDescriptor& inputTensorDesc,
                                              const TensorDescriptor& filterDesc) const
{
    auto dims = this->GetForwardOutputDim(inputTensorDesc, filterDesc);
    return TensorDescriptor(
        inputTensorDesc.GetType(),
        {std::get<0>(dims), std::get<1>(dims), std::get<2>(dims), std::get<3>(dims)});
}

TensorDescriptor
RNNDescriptor::GetBackwardOutputTensor(const TensorDescriptor& outputTensorDesc,
                                               const TensorDescriptor& filterDesc) const
{
    auto dims = this->GetBackwardOutputDim(outputTensorDesc, filterDesc);
    return TensorDescriptor(
        outputTensorDesc.GetType(),
        {std::get<0>(dims), std::get<1>(dims), std::get<2>(dims), std::get<3>(dims)});
}

TensorDescriptor
RNNDescriptor::GetBackwardWeightsTensor(const TensorDescriptor& inputTensorDesc,
                                                const TensorDescriptor& outputTensorDesc) const
{
    auto dims = this->GetBackwardsWeightsDim(inputTensorDesc, outputTensorDesc);
    return TensorDescriptor(
        outputTensorDesc.GetType(),
        {std::get<0>(dims), std::get<1>(dims), std::get<2>(dims), std::get<3>(dims)});
}

size_t RNNDescriptor::BackwardDataGetWorkSpaceSizeGEMM(Handle& handle,
                                                               const TensorDescriptor& wDesc,
                                                               const TensorDescriptor& dyDesc) const
{
    int out_h, out_w;
    std::tie(std::ignore, std::ignore, out_h, out_w) = miopen::tie4(dyDesc.GetLengths());
    int wei_c, wei_h, wei_w;
    std::tie(std::ignore, wei_c, wei_h, wei_w) = miopen::tie4(wDesc.GetLengths());
    size_t gemm_size = wei_c * wei_h * wei_w * out_h * out_w * sizeof(dyDesc.GetType());

    // gfx803 devices have limited memory
    // TODO: be graceful, need to ensure we can execute a config on the GPU
    // what if both the algos require > (1 << 30) memory
    if(handle.GetDeviceName() == "gfx803" && gemm_size > (1 << 30))
        gemm_size = 0;

    return (wei_h == 1 && wei_w == 1 && v == 1 && u == 1) ? 0 : gemm_size;
}

size_t RNNDescriptor::BackwardWeightsGetWorkSpaceSizeGEMM(
    Handle& handle, const TensorDescriptor& dyDesc, const TensorDescriptor& dwDesc) const
{
    int out_h, out_w;
    std::tie(std::ignore, std::ignore, out_h, out_w) = miopen::tie4(dyDesc.GetLengths());
    int wei_c, wei_h, wei_w;
    std::tie(std::ignore, wei_c, wei_h, wei_w) = miopen::tie4(dwDesc.GetLengths());
    size_t gemm_size = wei_c * wei_h * wei_w * out_h * out_w * sizeof(dyDesc.GetType());

    // gfx803 devices have limited memory
    // TODO: be graceful, need to ensure we can execute a config on the GPU
    // what if both the algos require > (1 << 30) memory
    if(handle.GetDeviceName() == "gfx803" && gemm_size > (1 << 30))
        gemm_size = 0;

    return gemm_size;
}

size_t
RNNDescriptor::BackwardWeightsGetWorkSpaceSizeDirect(Handle& handle,
                                                             const TensorDescriptor& dyDesc,
                                                             const TensorDescriptor& xDesc,
                                                             const TensorDescriptor& dwDesc) const
{
    mlo_construct_BwdWrW2D construct_params(0); // backward with regards to weights
    construct_params.doSearch(false);
    construct_params.setStream(&handle);
    construct_params.setOutputDescFromMLDesc(dyDesc);
    construct_params.setInputDescFromMLDesc(xDesc);
    construct_params.setWeightDescFromMLDesc(dwDesc);
    construct_params.setConvDescr(pad_h, pad_w, u, v, dilation_h, dilation_w);
    construct_params.mloConstruct();

    return construct_params.getWorkSpaceSzBytes();
}

size_t RNNDescriptor::ConvolutionBackwardWeightsGetWorkSpaceSize(
    Handle& handle,
    const TensorDescriptor& dyDesc,
    const TensorDescriptor& xDesc,
    const TensorDescriptor& dwDesc) const
{
    if(mode == miopenTranspose)
        return BackwardWeightsGetWorkSpaceSizeGEMM(handle, xDesc, dwDesc);

    return std::max(BackwardWeightsGetWorkSpaceSizeDirect(handle, dyDesc, xDesc, dwDesc),
                    BackwardWeightsGetWorkSpaceSizeGEMM(handle, dyDesc, dwDesc));
}
std::ostream& operator<<(std::ostream& stream, const RNNDescriptor& c)
{
    stream << c.pad_h << ", ";
    stream << c.pad_w << ", ";
    stream << c.u << ", ";
    stream << c.v << ", ";
    stream << c.dilation_h << ", ";
    stream << c.dilation_w << ", ";
    return stream;
}

*/

} // namespace miopen
//=======
#include <miopen/rnn.hpp>
#include <miopen/errors.hpp>
#include <miopen/env.hpp>

MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_AMD_ROCM_PRECOMPILED_BINARIES)
MIOPEN_DECLARE_ENV_VAR(MIOPEN_DEBUG_AMD_ASM_KERNELS_PERF_FILTERING)

// Disable specific warnings
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

namespace miopen {

RNNDescriptor::RNNDescriptor()
{
    seqLength = 0;
    nlayers   = 0;
    rnnMode   = miopenRNNRELU;
    dirMode   = miopenRNNunidirection;
    algoMode  = miopenRNNdefault;
    inputMode = miopenRNNlinear;
}

RNNDescriptor::RNNDescriptor(int sLength, int layer, miopenRNNDirectionMode_t bidir)
    : seqLength(sLength), nlayers(layer), dirMode(bidir)
{
    rnnMode   = miopenRNNRELU;
    algoMode  = miopenRNNdefault;
    inputMode = miopenRNNlinear;
}

RNNDescriptor::RNNDescriptor(miopenRNNMode_t p_mode,
                             int sLen,
                             int layer,
                             miopenRNNDirectionMode_t bidir)
    : seqLength(sLen), nlayers(layer), rnnMode(p_mode), dirMode(bidir)
{
    algoMode  = miopenRNNdefault;
    inputMode = miopenRNNlinear;
}

RNNDescriptor::RNNDescriptor(int hsz,
                             int layers,
                             miopenRNNInputMode_t inMode,
                             miopenRNNDirectionMode_t bidir,
                             miopenRNNMode_t rmode,
                             miopenRNNAlgo_t amode,
                             miopenDataType_t dType)
{
    hsize = seqLength = hsz;
    nlayers           = layers;

    inputMode = inMode;
    dirMode   = bidir;
    rnnMode   = rmode;
    algoMode  = amode;
    dataType  = dType;
}



size_t RNNDescriptor::GetWorkspaceSize(Handle& handle,
                                const int sLen,
                                TensorDescriptor** xDesc) const 
{
    size_t x = 0;
    return x;
}



size_t RNNDescriptor::GetReserveSize(Handle& handle,
                                const int sLen,
                                TensorDescriptor** xDesc) const
{
    size_t x = 0;
    return x;
}


size_t RNNDescriptor::GetParamsSize(Handle& handle,
                                    const TensorDescriptor& xDesc,
                                    miopenDataType_t dtype) const
{
    size_t x = 0;
    return x;
}

void RNNDescriptor::GetLayerParam(Handle& handle,
                                  const TensorDescriptor& xDesc,
                                  const TensorDescriptor& wDesc,
                                  ConstData_t w,
                                  const int layerID,
                                  const TensorDescriptor& paramDesc,
                                  Data_t** layerParam) const
{
}

void RNNDescriptor::GetLayerBias(Handle& handle,
                                 const TensorDescriptor& xDesc,
                                 const TensorDescriptor& wDesc,
                                 ConstData_t w,
                                 const int layerID,
                                 const TensorDescriptor& biasDesc,
                                 Data_t** layerBias) const
{
}

void RNNDescriptor::ForwardRNNTrain(Handle& handle,
                             const TensorDescriptor& xDesc,
                             ConstData_t x,
                             const TensorDescriptor& hxDesc,
                             ConstData_t hx,
                             const TensorDescriptor& cxDesc,
                             ConstData_t cx,
                             const TensorDescriptor& wDesc,
                             ConstData_t w,
                             const TensorDescriptor& yDesc,
                             Data_t y,
                             const TensorDescriptor& hyDesc,
                             Data_t hy,
                             const TensorDescriptor& cyDesc,
                             Data_t cy,
                             Data_t workSpace,
                             size_t workSpaceSize,
                             Data_t reserveSpace,
                             size_t reserveSpaceSize) const
{
}

//Assuming sequence length is set to > 0 otherwise throw exception.
void RNNDescriptor::ForwardRNNInference(Handle& handle,
                             const TensorDescriptor& xDesc,
                             ConstData_t x,
                             const TensorDescriptor& hxDesc,
                             ConstData_t hx,
                             const TensorDescriptor& cxDesc,
                             ConstData_t cx,
                             const TensorDescriptor& wDesc,
                             ConstData_t w,
                             const TensorDescriptor& yDesc,
                             Data_t y,
                             const TensorDescriptor& hyDesc,
                             Data_t hy,
                             const TensorDescriptor& cyDesc,
                             Data_t cy,
                             Data_t workSpace,
                             size_t workSpaceSize) const
{
}


void RNNDescriptor::ForwardRNNInferCell(Handle& handle,
                                        const TensorDescriptor& xDesc,
                                        ConstData_t x,
                                        const TensorDescriptor& hxDesc,
                                        ConstData_t hx,
                                        const TensorDescriptor& wDesc,
                                        ConstData_t w,
                                        const TensorDescriptor& yDesc,
                                        Data_t y,
                                        const TensorDescriptor& hyDesc,
                                        Data_t hy,
                                        Data_t workSpace,
                                        size_t workSpaceSize) const
{
}

void RNNDescriptor::ForwardRNNTrainCell(Handle& handle,
                                        const TensorDescriptor& xDesc,
                                        ConstData_t x,
                                        const TensorDescriptor& hxDesc,
                                        ConstData_t hx,
                                        const TensorDescriptor& wDesc,
                                        ConstData_t w,
                                        const TensorDescriptor& yDesc,
                                        Data_t y,
                                        const TensorDescriptor& hyDesc,
                                        Data_t hy,
                                        Data_t workSpace,
                                        size_t workSpaceSize,
                                        Data_t reserveSpace,
                                        size_t reserveSpaceSize) const
{
}

void RNNDescriptor::BackwardRNNDataCell(Handle& handle,
                                        const TensorDescriptor& yDesc,
                                        ConstData_t y,
                                        const TensorDescriptor& dyDesc,
                                        ConstData_t dy,
                                        const TensorDescriptor& dhyDesc,
                                        ConstData_t dhy,
                                        const TensorDescriptor& wDesc,
                                        ConstData_t w,
                                        const TensorDescriptor& hxDesc,
                                        ConstData_t hx,
                                        const TensorDescriptor& dxDesc,
                                        Data_t dx,
                                        const TensorDescriptor& dhxDesc,
                                        Data_t dhx,
                                        Data_t workSpace,
                                        size_t workSpaceSize,
                                        ConstData_t reserveSpace,
                                        size_t reserveSpaceSize) const
{
}

void RNNDescriptor::BackwardRNNWeightsCell(Handle& handle,
                                           const TensorDescriptor& xDesc,
                                           ConstData_t x,
                                           const TensorDescriptor& hxDesc,
                                           ConstData_t hx,
                                           const TensorDescriptor& yDesc,
                                           ConstData_t y,
                                           const TensorDescriptor& dwDesc,
                                           Data_t dw,
                                           ConstData_t workSpace,
                                           size_t workSpaceSize,
                                           ConstData_t reserveSpace,
                                           size_t reserveSpaceSize) const
{
}

} // namespace miopen

// Restore warnings
#ifdef __clang__
#pragma clang diagnostic pop
#endif
//>>>>>>> rnn
