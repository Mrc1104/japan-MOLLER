#pragma once
#include <iostream>
#include <cadef.h>
#include <vector>
#include <list>
#include "ErrorHandling.h"
#include "EpicChannel.h"

// Make this a singleton?

class EpicHandler
{
	using ChanList = std::vector<std::unique_ptr<EpicChannel>>;
	ChanList channels;
public:
	EpicHandler();
	~EpicHandler();
	EpicHandler(EpicHandler const&) = delete;
	EpicHandler& operator=(EpicHandler const&) = delete;
	EpicHandler(EpicHandler &&) noexcept = default;
	EpicHandler& operator=(EpicHandler &&) noexcept = default;
public:
	EpicChannel* ConnectChannel(char const* pv_name, ::capri priority = CA_PRIORITY_DEFAULT);
	void GetStatus(unsigned level=0);
	void CheckConnection(std::ostream& out = std::cout);
};

