#include <stdio.h>
#include <klib.h>

union scan {
	int c;
	char ch[2];
} sc;


int file_count;

void main()
{
	int sta = 0;
	int sel = 0;

	VideoMode();
	Start();
	SetCursorSize(6, 7);

	ScreenClear(0, 0, 79, 24);

	DrawLine(10, 2, 64, 17, 10);

	file_count = FetchFile();
	DisplayFile(sta, sel);
	DisplayInfo();

	while (1)
	{
		sc.c = GetKey();

		if (sc.ch[0] == ESC)
		{
			End();
		}
		else if (sc.ch[0] == RETURN)
		{
			if (IsDir())
			{
				ChangeDir();
				file_count = FetchFile();
				sta = sel = 0;
				DisplayFile(sta, sel);
				DisplayInfo();
			}
			else
			{
				FileOperator(14, 19, 240);
			}
			continue;
		}

		switch ((int)sc.ch[1])
		{
			case UP:
				sel--;

				if (sel < 0) sel = 0;
				if (sel < sta) sta--;

				DisplayFile(sta, sel);

				break;
			case DOWN:
				sel++;

				if (sel >= PAGELINE)
				{
					sta++;
					if (sta > file_count - PAGELINE)
					{
						sta = file_count - PAGELINE;
					}

				}

				if (sel > file_count - 1)
				{
					sel = file_count - 1;
				}

				DisplayFile(sta, sel);

				break;
			case LEFT:

				break;
			case RIGHT:

				break;
			default:
				break;
		}
	}
}