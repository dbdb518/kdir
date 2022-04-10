#include <dos.h>
#include <dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <klib.h>
#include <fcntl.h>
#include <sys/stat.h>

struct file_info {
	char name[13];
	char attr;
	char at[5];
	long size;
	struct file_info *next;
};

union scancode {
	int c;
	char ch[2];
} wkey;

char far *base_mem;
struct file_info *file_list, *file_first;
int fcount;
unsigned char *win_buf;
char nowdir[70];

void VideoMode()
{
	union REGS regs;
	int vmode;

	regs.h.ah = 0xf;

	vmode = int86(0x10, &regs, &regs) & 0xff;

	if (vmode != 2 && vmode !=3 && vmode != 7)
	{
		exit(1);
	}
	if (vmode == 7)
		base_mem = (char far *)0xB0000000;
	else
		base_mem = (char far *)0xB8000000;
}

void Start()
{
	memset(nowdir, 0x00, 70);
	getcwd(nowdir, 70);
}

void SetCursorSize(int high, int low)
{
	union REGS regs;
	regs.h.ah = 1;
	regs.h.ch = high;
	regs.h.cl = low;
	int86(0x10, &regs, &regs);
}

void SetCursorPosition(int x, int y)
{
	union REGS regs;
	regs.h.ah = 2;
	regs.h.dh = y;
	regs.h.dl = x;
	regs.h.bh = 0;
	int86(0x10, &regs, &regs);
}

void ScreenClear(int scol, int srow, int ecol, int erow)
{
	int i, j;
	char far *v;

	for (i = srow; i <= erow; i++)
	{
		for (j = scol; j <= ecol; j++)
		{
			v = base_mem + (i * 160) + (j * 2);
			*v = ' ';
			v++;
			*v = 0x7;
		}
	}
}

void Write_char(int x, int y, int c, int attr)
{
	char far *v;
	v = base_mem + (y * 160) + (x * 2);
	*v = c;
	v++;
	*v = attr;
}

void Write_str(int x, int y, char *p, int attr)
{
	char far *v;
	v = base_mem + (y * 160) + (x * 2);
	while (*p)
	{
		*v++ = *p++;
		*v++ = attr;
	}
}

void DrawLine(int sx, int sy, int ex, int ey, int attr)
{
	int i, j;

	Write_char(sx, sy, 201, attr);
	Write_char(ex, sy, 187, attr);
	Write_char(sx, ey, 200, attr);
	Write_char(ex, ey, 188, attr);

	for (i = sx + 1; i < ex; i++)
	{
		Write_char(i, sy, 205, attr);
		Write_char(i, ey, 205, attr);
	}

	for (j = sy + 1; j < ey; j++)
	{
		Write_char(sx, j, 186, attr);
		Write_char(ex, j, 186, attr);
	}
}

int FetchFile()
{
	struct ffblk f;
	int result;

	file_first = file_list = malloc(sizeof(struct file_info));

	strcpy(file_list->name, "No Files");
	strcpy(file_list->at, "-");
	file_list->size = 0;

	result = findfirst("*.*", &f, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCH | FA_DIREC);

	fcount = 0;

	while (!result)
	{
		if (strcmp(f.ff_name, "."))
		{
			strcpy(file_list->name, f.ff_name);
			file_list->attr = f.ff_attrib;
			file_list->size = f.ff_fsize;

			strcpy(file_list->at, "....");

			if (f.ff_attrib & FA_RDONLY) file_list->at[0] = 'R';
			if (f.ff_attrib & FA_HIDDEN) file_list->at[1] = 'H';
			if (f.ff_attrib & FA_SYSTEM) file_list->at[2] = 'S';
			if (f.ff_attrib & FA_ARCH  ) file_list->at[3] = 'A';
			if (f.ff_attrib & FA_DIREC ) strcpy(file_list->at, "*DIR");

			file_list = file_list->next = malloc(sizeof(struct file_info));
			fcount++;
		}
		result = findnext(&f);
	}

	file_list->next = NULL;

	return fcount;
}

void SelectFilePointer(int sel)
{
	int i;
	file_list = file_first;

	for (i = 0; i < sel; i++)
		file_list = file_list->next;
}

void DisplayFile(int sta, int sel)
{
	int i = 0;
	int j = 0;
	char size[10];
	char data[53];

	ScreenClear(11, 3, 63, 16);
	SelectFilePointer(sta);


	for (i = sta; i < sta + PAGELINE; i++)
	{
		Write_str(12, 3 + j, file_list->name, (file_list->attr & FA_DIREC ? 13 : 11));
		Write_str(40, 3 + j, file_list->at, 10);

		sprintf(size, "%6ld", file_list->size);
		Write_str(56, 3 + j, size, 14);
		j++;
		if (file_list->next)
			file_list = file_list->next;
		else
			break;
	}

	SelectFilePointer(sel);
	sprintf(data, " %-13s               %4s            %6ld  ", file_list->name, file_list->at, file_list->size);
	Write_str(11, 3 + sel - sta, data, 0x29);

}

int GetKey()
{
	union REGS regs;
	regs.h.ah = 0;
	return int86(0x16, &regs, &regs);
}

void End()
{
	FreeFile();
	chdir(nowdir);
	exit(1);
}

void FreeFile()
{
	file_list = file_first;

	while (file_list)
	{
		free(file_list);
		file_list = file_list->next;
	}
}

BOOL IsDir()
{
	return (file_list->attr & FA_DIREC);
}

void FileOperator(int x, int y, int attr)
{
	char input[MAXLENGTH];
	int count = 0;
	int i;

	ShowFileWindow(x, y, attr);

	memset(input, 0x00, MAXLENGTH);

	while (1)
	{
		for (i = 0; i < MAXLENGTH; i++)
			Write_char(x + 18 + i, y + 2, ' ', 42);

		Write_str(x + 18, y + 2, input, 46);

		wkey.c = GetKey();

		if (wkey.ch[0] == ESC)
		{
			break;
		}
		else if (wkey.ch[0] == RETURN)
		{
			break;
		}
		else if (wkey.ch[1] == F1)
		{
			/* Copy */
			Copy(input);
			Confirm(x, y, attr, 1);

			break;
		}
		else if (wkey.ch[1] == F2)
		{
			/* Rename */
			ChangeName(input);
			Confirm(x, y, attr, 2);
			break;
		}
		else if (wkey.ch[1] == F3)
		{
			/* Delete */
			Delete();
			Confirm(x, y, attr, 3);
			break;
		}
		else if (wkey.ch[1] == BS)
		{
			if (count > 0)
			{
				count--;
				input[count] = '\0';
				SetCursorPosition(x + 18 + count, y + 2);

			}
			continue;
		}

		if ((wkey.ch[0] >= 48 && wkey.ch[0] <= 57) ||
			(wkey.ch[0] >= 65 && wkey.ch[0] <= 90) ||
			(wkey.ch[0] >= 97 && wkey.ch[0] <=122) ||
			 wkey.ch[0] == 46)
		{
			if (count < MAXLENGTH - 1)
			{
				input[count] = wkey.ch[0];
				count++;
				input[count] = '\0';
				SetCursorPosition(x + 18 + count, y + 2);
			}
		}

	}

	HideFileWindow(x, y);
}

void ShowFileWindow(int x, int y, int attr)
{
	/* Save the Current Screen and Repaint */
	int i, j;
	char far *v;
	unsigned char *t;

	win_buf = (unsigned char *)malloc(HWINDOW * WWINDOW * 2);

	if (!win_buf)
	{
		Write_str(0, 5, "Memory Error", 7);
		exit(-1);
	}

	t = win_buf;

	for (i = y; i < y + HWINDOW + 1; i++)
	{
		for (j = x; j < x + WWINDOW + 1; j++)
		{
			v = base_mem + (i * 160) + (j * 2);
			*t++ = *v;
			*v = ' ';
			v++;
			*t++ = *v;
			*v = attr;
		}
	}

	/* Draw Line */
	i = 0;

	for (i = y + 1; i < (y + HWINDOW); i++)
	{
		Write_char(x, i, 186, 14);
		Write_char(x + WWINDOW, i, 186, 14);
	}

	j = 0;

	for (j = x + 1; j < (x + WWINDOW); j++)
	{
		Write_char(j, y, 205, 14);
		Write_char(j, y + HWINDOW, 205, 14);
	}

	Write_char(x, y, 201, 14);
	Write_char(x + WWINDOW, y, 187, 14);
	Write_char(x, y + HWINDOW, 200, 14);
	Write_char(x + WWINDOW, y + HWINDOW, 188, 14);

	/* Draw Title */
	Write_str(x + 2, y + 1, "Current Name", 116);
	Write_str(x + 2, y + 2, "    New Name", 116);

	Write_str(x + 18, y + 1, file_list->name, 115);
	Write_str(x + 2, y + 4, "Copy(F1)  Rename(F2)  Delete(F3)  Exit(ESC)", 113);

	/* Set Cursor */
	for (i = 0; i < MAXLENGTH; i++)
	{
		Write_char(x + 18 + i, y + 2, ' ', 42);
	}
	SetCursorPosition(x + 18, y + 2);


}

void HideFileWindow(int x, int y)
{
	int i, j;
	char far *v;
	unsigned char *t;
	t = win_buf;

	i = 0;
	j = 0;

	for (i = y; i < y + HWINDOW + 1; i++)
	{
		for (j = x; j < x + WWINDOW + 1; j++)
		{
			v = base_mem + (i * 160) + (j * 2);

			*v++ = *t++;
			*v = *t++;
		}
	}

	free(win_buf);

}

void ChangeDir()
{
	chdir(file_list->name);
}

void DisplayInfo()
{
	char buf[50];
	getcwd(buf, 50);

	Write_str(25, 1, "                        ", 49);
	Write_str(28, 1, buf, 53);

	sprintf(buf, "Total %d Files", fcount);
	Write_str(28, 18, buf, 12);
}

void Copy(char *input)
{
	int source, new;
	int limit;
	int i, count;
	char buf[1024];

	if (input[0] == '\0')
	{
		printf("new file name is empty.");
		return;
	}

	source = open(file_list->name, O_RDONLY | O_BINARY);
	new = open(input, O_CREAT | O_TRUNC | O_RDWR | O_BINARY | S_IREAD | S_IWRITE);

	if (source == -1 || new == -1)
	{
		printf("error while opening files");
		return;
	}

	limit = filelength(source) / 1024L;

	for (i = 0; i <= limit; i++)
	{
		count = read(source, buf, 1024);
		if (count <= 0)
		{
			printf("read count is zero");
			break;
		}

		if (write(new, buf, count) == -1)
		{
			printf("error while writing files");
			break;
		}
	}
}

void ChangeName(char *input)
{
	rename(file_list->name, input);
}

void Delete()
{
	remove(file_list->name);
}

void Confirm(int x, int y, int attr, int action)
{
	int i, j;
	char far *v;
	char buf[30];

	for (i = y + 1; i < y + HWINDOW; i++)
	{
		for (j = x + 1; j < x + WWINDOW - 1; j++)
		{
			v = base_mem + (i * 160) + (j * 2);
			*v++ = ' ';
			*v = attr;
		}
	}

	switch(action)
	{
		case 1:
			Write_str(x + 17, y + 2, "File Copied!", 113);
			break;
		case 2:
			Write_str(x + 17, y + 2, "File Renamed!", 113);
			break;
		case 3:
			Write_str(x + 17, y + 2, "File Deleted!", 113);
			break;
	}

	Write_str(x + 13, y + 4, "Press <Enter> to Quit", attr + 1);

	while (1)
	{
		wkey.c = GetKey();

		if (wkey.ch[0] == RETURN) break;
	}
}

