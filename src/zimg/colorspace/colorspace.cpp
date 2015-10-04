#include <algorithm>
#include "common/except.h"
#include "common/linebuffer.h"
#include "common/pixel.h"
#include "colorspace.h"
#include "colorspace_param.h"
#include "graph.h"

namespace zimg {;
namespace colorspace {;

ColorspaceConversion::ColorspaceConversion(unsigned width, unsigned height, const ColorspaceDefinition &in, const ColorspaceDefinition &out, CPUClass cpu)
try :
	m_width{ width },
	m_height{ height }
{
	for (const auto &func : get_operation_path(in, out)) {
		m_operations.emplace_back(func(cpu));
	}
} catch (const std::bad_alloc &) {
	throw error::OutOfMemory{};
}

graph::ImageFilter::filter_flags ColorspaceConversion::get_flags() const
{
	filter_flags flags{};

	flags.same_row = true;
	flags.in_place = true;
	flags.color = true;

	return flags;
}

graph::ImageFilter::image_attributes ColorspaceConversion::get_image_attributes() const
{
	return{ m_width, m_height, PixelType::FLOAT };
}

void ColorspaceConversion::process(void *, const graph::ImageBufferConst &src, const graph::ImageBuffer &dst, void *, unsigned i, unsigned left, unsigned right) const
{
	const float *src_ptr[3];
	float *dst_ptr[3];

	for (unsigned p = 0; p < 3; ++p) {
		src_ptr[p] = LineBuffer<const float>{ src, p }[i];
		dst_ptr[p] = LineBuffer<float>{ dst, p }[i];
	}

	if (m_operations.empty()) {
		for (unsigned p = 0; p < 3; ++p) {
			std::copy(src_ptr[p] + left, src_ptr[p] + right, dst_ptr[p] + left);
		}
	} else {
		m_operations[0]->process(src_ptr, dst_ptr, left, right);

		for (size_t i = 1; i < m_operations.size(); ++i) {
			m_operations[i]->process(dst_ptr, dst_ptr, left, right);
		}
	}
}

} // namespace colorspace
} // namespace zimg