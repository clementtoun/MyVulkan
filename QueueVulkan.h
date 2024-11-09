#pragma once

#include <optional>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> computeFamily;

	bool isComplete() {
		return this->graphicsFamily.has_value() && this->presentFamily.has_value() && this->transferFamily.has_value() && this->computeFamily.has_value();
	}
};