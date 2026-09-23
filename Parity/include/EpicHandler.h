#pragma once
#include <iostream>
#include <cadef.h>
#include <vector>
#include <list>
#include "ErrorHandling.h"
#include "EpicChannel.h"

class EpicHandler
{
	using ChanList = std::vector<std::unique_ptr<EpicChannel>>;
	ChanList channels;
	EpicHandler();
	~EpicHandler();
	EpicHandler(EpicHandler const&) = delete;
	EpicHandler& operator=(EpicHandler const&) = delete;
	EpicHandler(EpicHandler &&) noexcept = delete;
	EpicHandler& operator=(EpicHandler &&) noexcept = delete;
public:
	static EpicHandler& getInstance();
public:
	EpicChannel* ConnectChannel(char const* pv_name, ::capri priority = CA_PRIORITY_DEFAULT);
	void GetStatus(unsigned level=0);
	void CheckConnection(std::ostream& out = std::cout);
};

