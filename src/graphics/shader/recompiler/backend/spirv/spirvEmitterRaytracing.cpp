// raytracing: begin - software emulation of the hardware BVH node intersection.
// Derived from AMD GPURT (MIT, (c) 2022 GPUOpen Drivers), which implements this instruction
// for the same hardware family, with the PS5 deltas described in raytracing/phase3-design.md.
#include "graphics/shader/recompiler/backend/spirv/spirvEmitterInternal.h"

#include <vector>

namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter {

void DefineBvhIntersect(EmitterState& state) {
	if (!state.program.info.uses_bvh) {
		return;
	}
	const auto result_type = TypeU32Vector(state, 4);
	const auto u32         = TypeU32(state);
	const auto f32         = TypeF32(state);

	std::vector<uint32_t> signature {result_type};
	for (uint32_t i = 0; i < BvhIntersectScalarArgs; i++) {
		signature.push_back(u32);
	}
	for (uint32_t i = 0; i < BvhIntersectFloatArgs; i++) {
		signature.push_back(f32);
	}
	const auto function_type    = state.builder.Type(OpTypeFunction, signature);
	state.bvh_intersect_function = state.builder.AllocateId();
	state.builder.AddName(state.bvh_intersect_function, "bvh_intersect_ray");
	state.builder.AddFunction({OpFunction, result_type, state.bvh_intersect_function,
	                           FunctionControlNone, function_type});
	for (uint32_t i = 0; i < BvhIntersectScalarArgs; i++) {
		state.builder.AddFunction({OpFunctionParameter, u32, state.builder.AllocateId()});
	}
	for (uint32_t i = 0; i < BvhIntersectFloatArgs; i++) {
		state.builder.AddFunction({OpFunctionParameter, f32, state.builder.AllocateId()});
	}
	EmitLabel(state, state.builder.AllocateId());

	// Traversal is not implemented yet: report every child as the invalid node so the guest
	// traversal unwinds. Replaced by the box and triangle paths in the following steps.
	const auto invalid = ConstantU32(state, 0xffffffffu);
	const auto result  = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpCompositeConstruct, result_type, result, invalid, invalid, invalid, invalid});
	state.builder.AddFunction({OpReturnValue, result});
	state.builder.AddFunction({OpFunctionEnd});
}

bool EmitValueRaytracing(ValueEmitContext& ctx, const IR::Inst& inst) {
	if (inst.GetOpcode() != IR::ValueOpcode::BvhIntersectRay) {
		return false;
	}
	auto&                 state = ctx.state;
	std::vector<uint32_t> call {OpFunctionCall, TypeU32Vector(state, 4), state.builder.AllocateId(),
	                            state.bvh_intersect_function};
	const auto            result = call[2];
	for (uint32_t i = 0; i < inst.NumArgs(); i++) {
		call.push_back(ctx.Arg(inst, i));
	}
	state.builder.AddFunction(call);
	ctx.Define(inst, result);
	return true;
}

} // namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter
// raytracing: end
