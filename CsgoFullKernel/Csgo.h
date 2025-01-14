#pragma once
#include "Modules.h"
#include "Drawing.h"
#include "Mouse.h"
#include "Offsets.h"
#include "Keymap.h"


void ExitThread()
{
	Print("exiting systhem thread");
	if (isWin32Thread)
		UnspoofWin32Thread();

	PsTerminateSystemThread(STATUS_SUCCESS);
}

bool SaveWhileLoop()
{
	if (!UpdateKeyMap())
		ExitThread();

	if (!KeyDown(VK_NUMPAD5))
	{
		return true;
	}

	ExitThread();
}

Vector2 WorldToScreen(Vector3& pos, MAT4X4& g_viewMatrix)
{
	float _x = g_viewMatrix.c[0][0] * pos.x + g_viewMatrix.c[0][1] * pos.y + g_viewMatrix.c[0][2] * pos.z + g_viewMatrix.c[0][3];
	float _y = g_viewMatrix.c[1][0] * pos.x + g_viewMatrix.c[1][1] * pos.y + g_viewMatrix.c[1][2] * pos.z + g_viewMatrix.c[1][3];

	float w = g_viewMatrix.c[3][0] * pos.x + g_viewMatrix.c[3][1] * pos.y + g_viewMatrix.c[3][2] * pos.z + g_viewMatrix.c[3][3];

	if (w < 0.01f)
		return { 0,0 };


	float inv_w = 1.f / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = targetWindowWidth * .5f;
	float y = targetWindowHeight * .5f;

	x += 0.5f * _x * targetWindowWidth + 0.5f;
	y -= 0.5f * _y * targetWindowHeight + 0.5f;

	return { x,y };
}

void CsgoMain()
{
	Print("csgo main");

	while (SaveWhileLoop())
	{

		if (!SpoofWin32Thread())
			continue;

		//check if cs focused
		if (!IsWindowFocused("csgo.exe"))
		{
			Print("cs not focused\n");
			Sleep(100);
			UnspoofWin32Thread();
			continue;
		}

		hdc = NtUserGetDC(0);
		if (!hdc)
		{
			Print("failed to get userdc");
			UnspoofWin32Thread();
			continue;
		}

		brush = NtGdiCreateSolidBrush(RGB(255, 0, 0), NULL);
		if (!brush)
		{
			Print("failed create brush");
			NtUserReleaseDC(hdc);
			UnspoofWin32Thread();
			continue;
		}


		DWORD localplayer = ReadMemory<DWORD>(clientBase + dwLocalPlayer);
		DWORD localTeam = ReadMemory<DWORD>(localplayer + m_iTeamNum);//fixed


		Vector3 punchAngle = ReadMemory<Vector3>(localplayer + m_aimPunchAngle);


		punchAngle.x = punchAngle.x * 12; punchAngle.y = punchAngle.y * 12;
		float x = targetWindowWidth / 2 - punchAngle.y;
		float y = targetWindowHeight / 2 + punchAngle.x;


		RECT rect = { x - 2, y - 2, x + 2, y + 2 };
		FrameRect(hdc, &rect, brush, 2);


		MAT4X4 viewMatrix = ReadMemory<MAT4X4>(clientBase + dwViewMatrix);

		for (size_t i = 0; i < 32; i++)
		{
			DWORD currEnt = ReadMemory<DWORD>(clientBase + dwEntityList + (i * 0x10));
			if (!currEnt)
				continue;

			int entHealth = ReadMemory<int>(currEnt + m_iHealth);
			if (0 >= entHealth)
				continue;

			DWORD dormant = ReadMemory<DWORD>(currEnt + m_bDormant);
			if (dormant)
				continue;

			DWORD teamNum = ReadMemory<DWORD>(currEnt + m_iTeamNum);
			if (teamNum == localTeam)
				continue;

			// Calculate the position of the player's feet and head on the screen
			Vector3 feetPos = ReadMemory<Vector3>(currEnt + m_vecOrigin);
			Vector2 feetPosScreen = WorldToScreen(feetPos, viewMatrix);

			DWORD bonePtr = ReadMemory<DWORD>(currEnt + m_dwBoneMatrix);
			MAT3X4 boneMatrix = ReadMemory<MAT3X4>(bonePtr + 0x30 * 8);
			Vector3 headPos = { boneMatrix.c[0][3], boneMatrix.c[1][3], boneMatrix.c[2][3] };
			Vector2 headPosScreen = WorldToScreen(headPos, viewMatrix);

			// Calculate the dimensions of the box based on the player's head and feet
			int height = headPosScreen.y - feetPosScreen.y;
			int width = height / 4;

			// Calculate the position and size of the box for the player's head
			float head_x = headPosScreen.x - width;
			float head_y = headPosScreen.y - height / 8;
			float head_w = height / 4;
			RECT headBox = { head_x + head_w, head_y + height / 4, head_x, head_y };

			// Calculate the position and size of the box for the whole player
			float player_x = feetPosScreen.x - width;
			float player_y = feetPosScreen.y;
			float player_w = width * 2;
			float player_h = height + (height / 4);
			RECT playerBox = { player_x + player_w, player_y + player_h, player_x, player_y };

			// Draw the boxes on the screen
			FrameRect(hdc, &playerBox, brush, 1);
			FrameRect(hdc, &headBox, brush, 1);
		}


		NtGdiDeleteObjectApp(brush);
		NtUserReleaseDC(hdc);


		UnspoofWin32Thread();
		YieldProcessor();
	}
}
