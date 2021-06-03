#pragma once

#include "VpnBackgroundTask.g.h"

namespace winrt::VpnDll::implementation
{
	struct VpnBackgroundTask : VpnBackgroundTaskT<VpnBackgroundTask>
	{
		VpnBackgroundTask() = default;

		void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance);
	};
}

namespace winrt::VpnDll::factory_implementation
{
	struct VpnBackgroundTask : VpnBackgroundTaskT<VpnBackgroundTask, implementation::VpnBackgroundTask>
	{
	};
}
