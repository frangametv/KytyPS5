// raytracing: begin - software emulation of the hardware BVH node intersection.
// Derived from AMD GPURT (MIT, (c) 2022 GPUOpen Drivers), which implements this instruction
// for the same hardware family; the PS5 deltas are described in raytracing/phase3-design.md.
#include "graphics/shader/recompiler/backend/spirv/spirvEmitterInternal.h"

#include <array>
#include <vector>

namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter {
namespace {

constexpr uint32_t NodeKindBoxFp16 = 4;
constexpr uint32_t NodeKindBoxFp32 = 5;
constexpr uint32_t NodeSize        = 64;
constexpr uint32_t InvalidNode     = 0xffffffffu;

uint32_t Op1(EmitterState& state, uint32_t opcode, uint32_t type, uint32_t a) {
	const auto result = state.builder.AllocateId();
	state.builder.AddFunction({opcode, type, result, a});
	return result;
}

uint32_t Op2(EmitterState& state, uint32_t opcode, uint32_t type, uint32_t a, uint32_t b) {
	const auto result = state.builder.AllocateId();
	state.builder.AddFunction({opcode, type, result, a, b});
	return result;
}

uint32_t Op3(EmitterState& state, uint32_t opcode, uint32_t type, uint32_t a, uint32_t b,
             uint32_t c) {
	const auto result = state.builder.AllocateId();
	state.builder.AddFunction({opcode, type, result, a, b, c});
	return result;
}

uint32_t ConstantAddress(EmitterState& state, uint64_t value) {
	return state.builder.Constant(
	    OpConstant, TypeDeviceAddress(state),
	    {static_cast<uint32_t>(value), static_cast<uint32_t>(value >> 32u)});
}

uint32_t IsNan(EmitterState& state, uint32_t value) {
	return Op2(state, OpFUnordNotEqual, TypeBool(state), value, value);
}

// IEEE minNum/maxNum: a NaN operand is ignored, so one degenerate axis cannot discard the others.
uint32_t FloatMin(EmitterState& state, uint32_t a, uint32_t b) {
	const auto less   = Op2(state, OpFOrdLessThan, TypeBool(state), a, b);
	const auto take_a = Op2(state, OpLogicalOr, TypeBool(state), less, IsNan(state, b));
	return Op3(state, OpSelect, TypeF32(state), take_a, a, b);
}

uint32_t FloatMax(EmitterState& state, uint32_t a, uint32_t b) {
	const auto greater = Op2(state, OpFOrdGreaterThan, TypeBool(state), a, b);
	const auto take_a  = Op2(state, OpLogicalOr, TypeBool(state), greater, IsNan(state, b));
	return Op3(state, OpSelect, TypeF32(state), take_a, a, b);
}

// Resolve one guest address to a device address through the page table, then load a dword at
// a byte offset that stays inside the same caching page as the resolved base.
uint32_t LoadDword(EmitterState& state, uint32_t device_base, uint32_t byte_offset) {
	const auto address = Op2(state, OpIAdd, TypeDeviceAddress(state), device_base,
	                         ConstantAddress(state, byte_offset));
	const auto pointer = Op1(state, OpConvertUToPtr, TypePhysicalU32Pointer(state), address);
	const auto value   = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpLoad, TypeU32(state), value, pointer, MemoryAccessAlignedMask, sizeof(uint32_t)});
	return value;
}

// A page that failed to resolve is device address 0. Loads are skipped for it, as the DMA path
// does, since the node data is discarded anyway once the result is marked unmapped.
std::vector<uint32_t> LoadDwords(EmitterState& state, uint32_t mapped,
                                 const std::vector<std::pair<uint32_t, uint32_t>>& sources) {
	const auto u32         = TypeU32(state);
	const auto load_label  = state.builder.AllocateId();
	const auto load_exit   = state.builder.AllocateId();
	const auto skip_label  = state.builder.AllocateId();
	const auto merge_label = state.builder.AllocateId();
	state.builder.AddFunction({OpSelectionMerge, merge_label, SelectionControlNone});
	state.builder.AddFunction({OpBranchConditional, mapped, load_label, skip_label});
	EmitLabel(state, load_label);
	std::vector<uint32_t> loaded;
	loaded.reserve(sources.size());
	for (const auto& [page, offset]: sources) {
		loaded.push_back(LoadDword(state, page, offset));
	}
	state.builder.AddFunction({OpBranch, load_exit});
	EmitLabel(state, load_exit);
	state.builder.AddFunction({OpBranch, merge_label});
	EmitLabel(state, skip_label);
	state.builder.AddFunction({OpBranch, merge_label});
	EmitLabel(state, merge_label);
	const auto zero = ConstantU32(state, 0);
	for (auto& value: loaded) {
		const auto result = state.builder.AllocateId();
		state.builder.AddFunction({OpPhi, u32, result, value, load_exit, zero, skip_label});
		value = result;
	}
	return loaded;
}

uint32_t ResolvePage(EmitterState& state, uint32_t address) {
	const auto result = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpFunctionCall, TypeDeviceAddress(state), result, state.bda_pointer_function, address});
	return result;
}

using Vec3 = std::array<uint32_t, 3>;

Vec3 Subtract(EmitterState& state, const Vec3& a, const Vec3& b) {
	const auto f32 = TypeF32(state);
	return {Op2(state, OpFSub, f32, a[0], b[0]), Op2(state, OpFSub, f32, a[1], b[1]),
	        Op2(state, OpFSub, f32, a[2], b[2])};
}

uint32_t Dot(EmitterState& state, const Vec3& a, const Vec3& b) {
	const auto f32 = TypeF32(state);
	const auto xy  = Op2(state, OpFAdd, f32, Op2(state, OpFMul, f32, a[0], b[0]),
	                     Op2(state, OpFMul, f32, a[1], b[1]));
	return Op2(state, OpFAdd, f32, xy, Op2(state, OpFMul, f32, a[2], b[2]));
}

Vec3 Cross(EmitterState& state, const Vec3& a, const Vec3& b) {
	const auto f32  = TypeF32(state);
	const auto term = [&](uint32_t i, uint32_t j) {
		return Op2(state, OpFSub, f32, Op2(state, OpFMul, f32, a[i], b[j]),
		           Op2(state, OpFMul, f32, a[j], b[i]));
	};
	return {term(1, 2), term(2, 0), term(0, 1)};
}

// Pick one of four values by node kind, which doubles as the index of the triangle within
// a fan node.
uint32_t SelectByKind(EmitterState& state, uint32_t kind, uint32_t a, uint32_t b, uint32_t c,
                      uint32_t d) {
	const auto bool_type = TypeBool(state);
	const auto type      = TypeF32(state);
	const auto is0       = Op2(state, OpIEqual, bool_type, kind, ConstantU32(state, 0));
	const auto is1       = Op2(state, OpIEqual, bool_type, kind, ConstantU32(state, 1));
	const auto is2       = Op2(state, OpIEqual, bool_type, kind, ConstantU32(state, 2));
	const auto cd        = Op3(state, OpSelect, type, is2, c, d);
	const auto bcd       = Op3(state, OpSelect, type, is1, b, cd);
	return Op3(state, OpSelect, type, is0, a, bcd);
}

Vec3 SelectVertexByKind(EmitterState& state, uint32_t kind, const Vec3& a, const Vec3& b,
                        const Vec3& c, const Vec3& d) {
	return {SelectByKind(state, kind, a[0], b[0], c[0], d[0]),
	        SelectByKind(state, kind, a[1], b[1], c[1], d[1]),
	        SelectByKind(state, kind, a[2], b[2], c[2], d[2])};
}

struct Ray {
	uint32_t extent;
	Vec3     origin;
	Vec3     direction;
	Vec3     inverse;
};

// One ray/AABB slab test. Returns the near distance and whether the box was hit, using the
// sign of the inverse direction to order each axis and growing the box by the descriptor's
// ULP allowance so it can never reject a ray the triangle test would have accepted.
struct SlabResult {
	uint32_t near_distance;
	uint32_t hit;
};

SlabResult Slab(EmitterState& state, const Ray& ray, const Vec3& box_min, const Vec3& box_max,
                uint32_t grow_scale) {
	const auto f32       = TypeF32(state);
	const auto bool_type = TypeBool(state);
	const auto zero      = ConstantF32(state, 0x00000000u);

	uint32_t near_max = 0;
	uint32_t far_min  = 0;
	for (uint32_t axis = 0; axis < 3u; axis++) {
		const auto lo =
		    Op2(state, OpFMul, f32, Op2(state, OpFSub, f32, box_min[axis], ray.origin[axis]),
		        ray.inverse[axis]);
		const auto hi =
		    Op2(state, OpFMul, f32, Op2(state, OpFSub, f32, box_max[axis], ray.origin[axis]),
		        ray.inverse[axis]);
		const auto positive =
		    Op2(state, OpFOrdGreaterThanEqual, bool_type, ray.inverse[axis], zero);
		const auto near = Op3(state, OpSelect, f32, positive, lo, hi);
		const auto far  = Op3(state, OpSelect, f32, positive, hi, lo);
		near_max        = axis == 0 ? near : FloatMax(state, near_max, near);
		far_min         = axis == 0 ? far : FloatMin(state, far_min, far);
	}

	// Only a NaN on every axis survives the aggregation; that interval is empty and misses.
	const auto nan =
	    Op2(state, OpLogicalOr, bool_type, IsNan(state, near_max), IsNan(state, far_min));
	const auto min_t = Op3(state, OpSelect, f32, nan, ConstantF32(state, 0x7f800000u),
	                       FloatMax(state, near_max, zero));
	const auto max_t = Op3(state, OpSelect, f32, nan, ConstantF32(state, 0xff800000u),
	                       FloatMin(state, far_min, ray.extent));
	const auto grown = Op2(state, OpFMul, f32, max_t, grow_scale);
	const auto hit   = Op2(state, OpFOrdLessThanEqual, bool_type, min_t, grown);
	return {min_t, hit};
}

// Fixed five-comparator network, PS5 order. The second clause of the swap condition is what
// migrates misses to the tail, so the returned four pointers have no holes.
void SortChildren(EmitterState& state, std::array<uint32_t, 4>& child, std::array<uint32_t, 4>& key,
                  uint32_t enabled) {
	static constexpr std::array<std::pair<uint32_t, uint32_t>, 5> network {
	    {{0, 1}, {2, 3}, {0, 2}, {1, 3}, {1, 2}}};
	const auto u32       = TypeU32(state);
	const auto f32       = TypeF32(state);
	const auto bool_type = TypeBool(state);
	const auto invalid   = ConstantU32(state, InvalidNode);
	for (const auto& [a, b]: network) {
		const auto b_valid     = Op2(state, OpINotEqual, bool_type, child[b], invalid);
		const auto nearer      = Op2(state, OpFOrdLessThan, bool_type, key[b], key[a]);
		const auto a_empty     = Op2(state, OpIEqual, bool_type, child[a], invalid);
		const auto swap        = Op2(state, OpLogicalOr, bool_type,
		                             Op2(state, OpLogicalAnd, bool_type, b_valid, nearer), a_empty);
		const auto active      = Op2(state, OpLogicalAnd, bool_type, swap, enabled);
		const auto new_a_child = Op3(state, OpSelect, u32, active, child[b], child[a]);
		const auto new_b_child = Op3(state, OpSelect, u32, active, child[a], child[b]);
		const auto new_a_key   = Op3(state, OpSelect, f32, active, key[b], key[a]);
		const auto new_b_key   = Op3(state, OpSelect, f32, active, key[a], key[b]);
		child[a]               = new_a_child;
		child[b]               = new_b_child;
		key[a]                 = new_a_key;
		key[b]                 = new_b_key;
	}
}

} // namespace

void DefineBvhIntersect(EmitterState& state) {
	if (!state.program.info.uses_bvh) {
		return;
	}
	const auto u32         = TypeU32(state);
	const auto f32         = TypeF32(state);
	const auto bool_type   = TypeBool(state);
	const auto address     = TypeDeviceAddress(state);
	const auto result_type = TypeU32Vector(state, 4);

	std::vector<uint32_t> signature {result_type};
	for (uint32_t i = 0; i < BvhIntersectScalarArgs; i++) {
		signature.push_back(u32);
	}
	for (uint32_t i = 0; i < BvhIntersectFloatArgs; i++) {
		signature.push_back(f32);
	}
	const auto function_type     = state.builder.Type(OpTypeFunction, signature);
	state.bvh_intersect_function = state.builder.AllocateId();
	state.builder.AddName(state.bvh_intersect_function, "bvh_intersect_ray");
	state.builder.AddFunction({OpFunction, result_type, state.bvh_intersect_function,
	                           FunctionControlNone, function_type});

	std::array<uint32_t, BvhIntersectScalarArgs> scalar {};
	for (auto& id: scalar) {
		id = state.builder.AllocateId();
		state.builder.AddFunction({OpFunctionParameter, u32, id});
	}
	std::array<uint32_t, BvhIntersectFloatArgs> real {};
	for (auto& id: real) {
		id = state.builder.AllocateId();
		state.builder.AddFunction({OpFunctionParameter, f32, id});
	}
	EmitLabel(state, state.builder.AllocateId());

	const auto invalid    = ConstantU32(state, InvalidNode);
	const auto all_missed = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpCompositeConstruct, result_type, all_missed, invalid, invalid, invalid, invalid});

	// Descriptor: base address in 256-byte units across dwords 0 and 1, box growing amount in
	// bits 62:55, box sorting enable in bit 63.
	const auto base_low = Op1(state, OpUConvert, address, scalar[0]);
	const auto base_high =
	    Op2(state, OpShiftLeftLogical, address,
	        Op1(state, OpUConvert, address,
	            Op2(state, OpBitwiseAnd, u32, scalar[1], ConstantU32(state, 0xffu))),
	        ConstantAddress(state, 32));
	const auto bvh_base =
	    Op2(state, OpShiftLeftLogical, address,
	        Op2(state, OpBitwiseOr, address, base_low, base_high), ConstantAddress(state, 8));
	const auto grow = Op2(state, OpBitwiseAnd, u32,
	                      Op2(state, OpShiftRightLogical, u32, scalar[1], ConstantU32(state, 23)),
	                      ConstantU32(state, 0xffu));
	const auto sort_enabled =
	    Op2(state, OpINotEqual, bool_type,
	        Op2(state, OpBitwiseAnd, u32, scalar[1], ConstantU32(state, 0x80000000u)),
	        ConstantU32(state, 0));
	// 1 + grow * 2^-24
	const auto grow_scale = Op2(state, OpFAdd, f32, ConstantF32(state, 0x3f800000u),
	                            Op2(state, OpFMul, f32, Op1(state, OpConvertUToF, f32, grow),
	                                ConstantF32(state, 0x33800000u)));

	const auto kind     = Op2(state, OpBitwiseAnd, u32, scalar[4], ConstantU32(state, 7));
	const auto node_low = Op1(state, OpUConvert, address, scalar[4]);
	const auto node_high =
	    Op2(state, OpShiftLeftLogical, address, Op1(state, OpUConvert, address, scalar[5]),
	        ConstantAddress(state, 32));
	const auto node_index =
	    Op2(state, OpShiftRightLogical, address,
	        Op2(state, OpBitwiseOr, address, node_low, node_high), ConstantAddress(state, 3));
	const auto node_address =
	    Op2(state, OpIAdd, address, bvh_base,
	        Op2(state, OpIMul, address, node_index, ConstantAddress(state, NodeSize)));

	const Ray ray {
	    .extent    = real[0],
	    .origin    = {real[1], real[2], real[3]},
	    .direction = {real[4], real[5], real[6]},
	    .inverse   = {real[7], real[8], real[9]},
	};

	// Two box-node encodings share the same intersection; only the load differs. A 128-byte
	// fp32 node spans two 64-byte units that may sit in different caching pages, so it needs
	// two page resolves; a 64-byte fp16 node needs one.
	const auto intersect = [&](std::array<uint32_t, 4> child, const std::array<Vec3, 4>& box_min,
	                           const std::array<Vec3, 4>& box_max, uint32_t mapped) {
		std::array<uint32_t, 4> key {};
		for (uint32_t slot = 0; slot < 4u; slot++) {
			const auto slab = Slab(state, ray, box_min[slot], box_max[slot], grow_scale);
			const auto live = Op2(state, OpLogicalAnd, bool_type, slab.hit, mapped);
			child[slot]     = Op3(state, OpSelect, u32, live, child[slot], invalid);
			key[slot]       = slab.near_distance;
		}
		SortChildren(state, child, key, sort_enabled);
		const auto composite = state.builder.AllocateId();
		state.builder.AddFunction(
		    {OpCompositeConstruct, result_type, composite, child[0], child[1], child[2], child[3]});
		return composite;
	};

	const auto zero_address = ConstantAddress(state, 0);
	const auto is_box32 =
	    Op2(state, OpIEqual, bool_type, kind, ConstantU32(state, NodeKindBoxFp32));
	const auto box32_label = state.builder.AllocateId();
	const auto rest_label  = state.builder.AllocateId();
	const auto merge_label = state.builder.AllocateId();
	const auto box32_exit  = state.builder.AllocateId();
	state.builder.AddFunction({OpSelectionMerge, merge_label, SelectionControlNone});
	state.builder.AddFunction({OpBranchConditional, is_box32, box32_label, rest_label});

	EmitLabel(state, box32_label);
	const auto page0 = ResolvePage(state, node_address);
	const auto page1 = ResolvePage(
	    state, Op2(state, OpIAdd, address, node_address, ConstantAddress(state, NodeSize)));
	const auto mapped32 =
	    Op2(state, OpLogicalAnd, bool_type, Op2(state, OpINotEqual, bool_type, page0, zero_address),
	        Op2(state, OpINotEqual, bool_type, page1, zero_address));
	std::vector<std::pair<uint32_t, uint32_t>> sources32;
	for (uint32_t index = 0; index < 28u; index++) {
		sources32.emplace_back(index < 16u ? page0 : page1, (index & 15u) * 4u);
	}
	const auto              loaded32 = LoadDwords(state, mapped32, sources32);
	const auto              dword32  = [&](uint32_t index) { return loaded32[index]; };
	std::array<uint32_t, 4> child32 {};
	std::array<Vec3, 4>     min32 {};
	std::array<Vec3, 4>     max32 {};
	for (uint32_t slot = 0; slot < 4u; slot++) {
		child32[slot]    = dword32(slot);
		const auto first = 4u + slot * 6u;
		for (uint32_t axis = 0; axis < 3u; axis++) {
			min32[slot][axis] = Op1(state, OpBitcast, f32, dword32(first + axis));
			max32[slot][axis] = Op1(state, OpBitcast, f32, dword32(first + 3u + axis));
		}
	}
	const auto box32_result = intersect(child32, min32, max32, mapped32);
	state.builder.AddFunction({OpBranch, box32_exit});
	EmitLabel(state, box32_exit);
	state.builder.AddFunction({OpBranch, merge_label});

	EmitLabel(state, rest_label);
	const auto is_box16 =
	    Op2(state, OpIEqual, bool_type, kind, ConstantU32(state, NodeKindBoxFp16));
	const auto box16_label = state.builder.AllocateId();
	const auto other_label = state.builder.AllocateId();
	const auto inner_merge = state.builder.AllocateId();
	const auto box16_exit  = state.builder.AllocateId();
	state.builder.AddFunction({OpSelectionMerge, inner_merge, SelectionControlNone});
	state.builder.AddFunction({OpBranchConditional, is_box16, box16_label, other_label});

	EmitLabel(state, box16_label);
	const auto page16   = ResolvePage(state, node_address);
	const auto mapped16 = Op2(state, OpINotEqual, bool_type, page16, zero_address);
	std::vector<std::pair<uint32_t, uint32_t>> sources16;
	for (uint32_t index = 0; index < 16u; index++) {
		sources16.emplace_back(page16, index * 4u);
	}
	const auto loaded16 = LoadDwords(state, mapped16, sources16);
	const auto dword16  = [&](uint32_t index) { return loaded16[index]; };
	// Each child packs six halves into three dwords: min = (lo0, hi0, lo1),
	// max = (hi1, lo2, hi2).
	const auto low_half  = [&](uint32_t word) { return EmitF16BitsToF32(state, word); };
	const auto high_half = [&](uint32_t word) {
		return EmitF16BitsToF32(state,
		                        Op2(state, OpShiftRightLogical, u32, word, ConstantU32(state, 16)));
	};
	std::array<uint32_t, 4> child16 {};
	std::array<Vec3, 4>     min16 {};
	std::array<Vec3, 4>     max16 {};
	for (uint32_t slot = 0; slot < 4u; slot++) {
		child16[slot]    = dword16(slot);
		const auto first = 4u + slot * 3u;
		const auto w0    = dword16(first);
		const auto w1    = dword16(first + 1u);
		const auto w2    = dword16(first + 2u);
		min16[slot]      = {low_half(w0), high_half(w0), low_half(w1)};
		max16[slot]      = {high_half(w1), low_half(w2), high_half(w2)};
	}
	const auto box16_result = intersect(child16, min16, max16, mapped16);
	state.builder.AddFunction({OpBranch, box16_exit});
	EmitLabel(state, box16_exit);
	state.builder.AddFunction({OpBranch, inner_merge});

	EmitLabel(state, other_label);
	// Kinds 0-3 are all triangle nodes, and the kind is also the index of the triangle
	// within the fan.
	const auto is_triangle =
	    Op2(state, OpUGreaterThan, bool_type, ConstantU32(state, NodeKindBoxFp16), kind);
	const auto tri_label  = state.builder.AllocateId();
	const auto none_label = state.builder.AllocateId();
	const auto tri_merge  = state.builder.AllocateId();
	const auto tri_exit   = state.builder.AllocateId();
	state.builder.AddFunction({OpSelectionMerge, tri_merge, SelectionControlNone});
	state.builder.AddFunction({OpBranchConditional, is_triangle, tri_label, none_label});

	EmitLabel(state, tri_label);
	const auto tri_page   = ResolvePage(state, node_address);
	const auto tri_mapped = Op2(state, OpINotEqual, bool_type, tri_page, zero_address);
	std::vector<std::pair<uint32_t, uint32_t>> tri_sources;
	for (uint32_t index = 0; index < 16u; index++) {
		tri_sources.emplace_back(tri_page, index * 4u);
	}
	const auto          tri_loaded = LoadDwords(state, tri_mapped, tri_sources);
	const auto          tri_dword  = [&](uint32_t index) { return tri_loaded[index]; };
	std::array<Vec3, 5> vertex {};
	for (uint32_t index = 0; index < 5u; index++) {
		for (uint32_t axis = 0; axis < 3u; axis++) {
			vertex[index][axis] = Op1(state, OpBitcast, f32, tri_dword(index * 3u + axis));
		}
	}
	// Five vertices encode four triangles as a fan centred on v2:
	// (v0,v1,v2) (v1,v3,v2) (v2,v3,v4) (v2,v4,v0). This is the PS5 delta - stock RDNA2
	// packs only two triangles into four vertices.
	const auto v0 = SelectVertexByKind(state, kind, vertex[0], vertex[1], vertex[2], vertex[2]);
	const auto v1 = SelectVertexByKind(state, kind, vertex[1], vertex[3], vertex[3], vertex[4]);
	const auto v2 = SelectVertexByKind(state, kind, vertex[2], vertex[2], vertex[4], vertex[0]);

	const auto e1      = Subtract(state, v1, v0);
	const auto e2      = Subtract(state, v2, v0);
	const auto e3      = Subtract(state, ray.origin, v0);
	const auto s1      = Cross(state, ray.direction, e2);
	const auto s2      = Cross(state, e3, e1);
	const auto t_num   = Dot(state, e2, s2);
	const auto t_denom = Dot(state, s1, e1);
	const auto i_num   = Dot(state, e3, s1);
	const auto j_num   = Dot(state, ray.direction, s2);

	const auto zero_f = ConstantF32(state, 0x00000000u);
	const auto one_f  = ConstantF32(state, 0x3f800000u);
	const auto t      = Op2(state, OpFDiv, f32, t_num, t_denom);
	const auto u      = Op2(state, OpFDiv, f32, i_num, t_denom);
	const auto v      = Op2(state, OpFDiv, f32, j_num, t_denom);
	auto       missed =
	    Op2(state, OpLogicalOr, bool_type, Op2(state, OpFOrdLessThan, bool_type, u, zero_f),
	        Op2(state, OpFOrdGreaterThan, bool_type, u, one_f));
	missed = Op2(state, OpLogicalOr, bool_type, missed,
	             Op2(state, OpFOrdLessThan, bool_type, v, zero_f));
	missed = Op2(state, OpLogicalOr, bool_type, missed,
	             Op2(state, OpFOrdGreaterThan, bool_type, Op2(state, OpFAdd, f32, u, v), one_f));
	missed = Op2(state, OpLogicalOr, bool_type, missed,
	             Op2(state, OpFOrdLessThan, bool_type, t, zero_f));
	missed =
	    Op2(state, OpLogicalOr, bool_type, missed, Op1(state, OpLogicalNot, bool_type, tri_mapped));
	// A miss is encoded in the numerator, not a separate flag.
	const auto infinity  = ConstantF32(state, 0x7f800000u);
	const auto out_t_num = Op3(state, OpSelect, f32, missed, infinity, t_num);
	const auto out_denom = Op3(state, OpSelect, f32, missed, one_f, t_denom);

	// Barycentric mode is descriptor bit 120, and is what the platform compiler emits by
	// default. The node's last dword then holds one byte per triangle: bits 0-1 select which
	// barycentric is reported first, bits 2-3 which is reported second. This undoes the vertex
	// rotation the builder applied so the pair matches the caller's own mesh winding.
	const auto id_word = tri_dword(15);
	const auto shift   = Op2(state, OpShiftLeftLogical, u32, kind, ConstantU32(state, 3));
	const auto mapping = Op2(state, OpShiftRightLogical, u32, id_word, shift);
	const auto pick    = [&](uint32_t offset) {
		const auto index =
		    Op2(state, OpBitwiseAnd, u32,
		        Op2(state, OpShiftRightLogical, u32, mapping, ConstantU32(state, offset)),
		        ConstantU32(state, 3));
		const auto b0   = Op2(state, OpFSub, f32, Op2(state, OpFSub, f32, out_denom, i_num), j_num);
		const auto is0  = Op2(state, OpIEqual, bool_type, index, ConstantU32(state, 0));
		const auto is1  = Op2(state, OpIEqual, bool_type, index, ConstantU32(state, 1));
		const auto rest = Op3(state, OpSelect, f32, is1, i_num, j_num);
		return Op3(state, OpSelect, f32, is0, b0, rest);
	};
	const auto barycentric_mode =
	    Op2(state, OpINotEqual, bool_type,
	        Op2(state, OpBitwiseAnd, u32, scalar[3], ConstantU32(state, 1u << 24u)),
	        ConstantU32(state, 0));
	// Triangle-ID mode instead reports the node's stored id plus the triangle index, and an
	// explicit hit flag.
	const auto triangle_id = Op2(state, OpIAdd, u32, id_word, kind);
	const auto hit_flag =
	    Op3(state, OpSelect, u32, missed, ConstantU32(state, 0), ConstantU32(state, 1));
	const auto third = Op3(state, OpSelect, u32, barycentric_mode,
	                       Op1(state, OpBitcast, u32, pick(0)), triangle_id);
	const auto fourth =
	    Op3(state, OpSelect, u32, barycentric_mode, Op1(state, OpBitcast, u32, pick(2)), hit_flag);
	const auto tri_result = state.builder.AllocateId();
	state.builder.AddFunction({OpCompositeConstruct, result_type, tri_result,
	                           Op1(state, OpBitcast, u32, out_t_num),
	                           Op1(state, OpBitcast, u32, out_denom), third, fourth});
	state.builder.AddFunction({OpBranch, tri_exit});
	EmitLabel(state, tri_exit);
	state.builder.AddFunction({OpBranch, tri_merge});

	EmitLabel(state, none_label);
	state.builder.AddFunction({OpBranch, tri_merge});

	EmitLabel(state, tri_merge);
	const auto tri_phi = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpPhi, result_type, tri_phi, tri_result, tri_exit, all_missed, none_label});
	state.builder.AddFunction({OpBranch, inner_merge});

	EmitLabel(state, inner_merge);
	const auto inner_result = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpPhi, result_type, inner_result, box16_result, box16_exit, tri_phi, tri_merge});
	state.builder.AddFunction({OpBranch, merge_label});

	EmitLabel(state, merge_label);
	const auto result = state.builder.AllocateId();
	state.builder.AddFunction(
	    {OpPhi, result_type, result, box32_result, box32_exit, inner_result, inner_merge});
	state.builder.AddFunction({OpReturnValue, result});
	state.builder.AddFunction({OpFunctionEnd});
}

bool EmitValueRaytracing(ValueEmitContext& ctx, const IR::Inst& inst) {
	if (inst.GetOpcode() != IR::ValueOpcode::BvhIntersectRay) {
		return false;
	}
	auto&                 state  = ctx.state;
	const auto            result = state.builder.AllocateId();
	std::vector<uint32_t> call {OpFunctionCall, TypeU32Vector(state, 4), result,
	                            state.bvh_intersect_function};
	for (uint32_t i = 0; i < inst.NumArgs(); i++) {
		call.push_back(ctx.Arg(inst, i));
	}
	state.builder.AddFunction(call);
	ctx.Define(inst, result);
	return true;
}

} // namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter
// raytracing: end
