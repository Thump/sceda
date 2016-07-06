#define PATCHLEVEL 0
/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994-1995  Stephen Chenney (stephen@cs.su.oz.au)
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program; if not, write to the Free Software
**    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* Layer structure. */
typedef struct _Layer {
	int		num;
	char	*name;
	Boolean	state;
	ObjectInstancePtr	*instances;
	int		num_instances;
	int		max_num;
	} Layer, *LayerPtr;

/*
**	Functions
*/
extern void	Add_New_Layer(int, char*, Boolean);
extern void	Layers_Init();
extern void	Layers_Create_Menu(WindowInfoPtr);
extern Boolean	Layer_Is_Visible(int);
extern void	Update_Visible_List(WindowInfoPtr);
extern void	Save_Layers(FILE*);
extern int	Layer_Get_Num();
extern void	Reset_Layers();
extern void	Layer_Add_Instance(LayerPtr, int, ObjectInstancePtr);
extern void	Layer_Remove_Instance(LayerPtr, int, ObjectInstancePtr);

