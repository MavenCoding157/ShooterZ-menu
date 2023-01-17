#include "hacks.h"
#include "globals.h"
#include "gui.h"

#include <thread>

//bool (or something)
int flashDur = 0;

void hacks::VisualsThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		
		if (!localPlayer)
			continue;

		const auto localPlayerFlags = mem.Read<std::uintptr_t>(localPlayer + offsets::m_fFlags);

		if (!localPlayerFlags)
			continue;

		//del
		const auto localHealth = mem.Read<std::int32_t>(localPlayer + offsets::m_iHealth);

		if (!localHealth)
			continue;

		const auto CrosshairID = mem.Read<std::int32_t>(localPlayer + offsets::m_iCrosshairId);

		if (CrosshairID || CrosshairID > 64)
			continue;
		//here

		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);

		if (!glowManager)
			continue;

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i)
		{	
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (!player)
				continue;
			
			const auto team = mem.Read<std::int32_t>(player + offsets::m_iTeamNum);
			
			if (team == localTeam)
				continue;
	
			const auto lifeState = mem.Read<std::int32_t>(player + offsets::m_lifeState);

			if (lifeState != 0)
				continue;

			if (globals::glow)
			{
				const auto glowIndex = mem.Read<std::int32_t>(player + offsets::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::glowColor[0]); //red
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::glowColor[1]); //blue
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::glowColor[2]); //green 
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::glowColor[3]); //alpha

				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x29, false);
			}

			if (globals::radar)
				mem.Write(player + offsets::m_bSpotted, true);

			if (globals::chams)
			{
				mem.Write(player + offsets::m_clrRender, true);
			}
			
			if (globals::bhop)
			{
				if (GetAsyncKeyState(VK_SPACE))
					(localPlayerFlags & (1 << 0)) ?
					mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceJump, 6) :
					mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceJump, 4);
			}

			if (globals::flashDur)
			{
				flashDur = mem.Read<int32_t>(localPlayer + offsets::m_flFlashDuration);
				if (flashDur > 0)
					mem.Write(localPlayer + offsets::m_flFlashDuration, 0);

				Sleep(1);
			}
		}	
	}
}