/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <migraphx/onnx/op_parser.hpp>
#include <migraphx/instruction.hpp>
#include <migraphx/ranges.hpp>
#include <migraphx/make_op.hpp>
#include <migraphx/tune_axis.hpp>
#include <migraphx/common.hpp>
#include <migraphx/onnx/quantize_dequantize_linear.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace onnx {

struct parse_quantizelinear : op_parser<parse_quantizelinear>
{
    std::vector<op_desc> operators() const { return {{"QuantizeLinear"}}; }

    instruction_ref parse(const op_desc& opd,
                          const onnx_parser& parser,
                          const onnx_parser::node_info& info,
                          std::vector<instruction_ref> args) const
    {
        if(args.size() < 2 or args.size() > 3)
        {
            MIGRAPHX_THROW("QuantizeLinear: must have either 2 or 3 inputs, " +
                           std::to_string(args.size()) + " input(s) provided");
        }

        // Starting with version 19 ONNX introduced the constraint that x and y_scale types must be
        // the same
        if(parser.opset_version >= 19 and
           args[0]->get_shape().type() != args[1]->get_shape().type())
        {
            MIGRAPHX_THROW("QuantizeLinear: x and y_scale must be of same type");
        }

        if(args.size() == 3 and args[1]->get_shape().lens() != args[2]->get_shape().lens())
        {
            MIGRAPHX_THROW(
                "QuantizeLinear: y_scale and y_zero_point shapes must be equal. Provided y_scale "
                "shape: " +
                to_string_range(args[1]->get_shape().lens()) +
                ", provided y_zero_point shape: " + to_string_range(args[2]->get_shape().lens()));
        }

        int axis = 1;
        if(contains(info.attributes, "axis"))
            axis = info.attributes.at("axis").i();

        int block_size = 0;
        if(contains(info.attributes, "block_size"))
            block_size = info.attributes.at("block_size").i();

        std::optional<migraphx::shape::type_t> output_type;
        if(contains(info.attributes, "output_dtype"))
        {
            output_type = get_type(info.attributes.at("output_dtype").i());
        }

        if(output_type.has_value() and args.size() == 3 and
           *output_type != args[2]->get_shape().type())
        {
            MIGRAPHX_THROW(
                "QuantizeLinear: output_type and y_zero_point type must match. output_type: " +
                to_string(*output_type) +
                +", y_zero_point type: " + to_string(args[2]->get_shape().type()));
        }

        args = transform_quantize_dequantize_linear_inputs(
            info, opd.onnx_name, block_size, axis, args);

        if(parser.opset_version < 19)
        {
            auto common_type = common_shape({args[0]->get_shape(), args[1]->get_shape()}).type();
            std::transform(args.begin(), args.begin() + 2, args.begin(), [&](auto ins) {
                if(ins->get_shape().type() != common_type)
                    ins = info.add_instruction(make_op("convert", {{"target_type", common_type}}),
                                               ins);
                return ins;
            });
        }

        if(output_type.has_value())
            return info.add_instruction(make_op("quantizelinear", {{"out_type", *output_type}}),
                                        args);
        else
            return info.add_instruction(make_op("quantizelinear"), args);
    }
};

} // namespace onnx
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
