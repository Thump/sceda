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

#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	proto.h: Function prototypes.
**
**	Created: 05/03/94
**
*/

#ifndef __SCED_PROTO__
#define __SCED_PROTO__


/*
**	Functions from viewport.c
*/
extern Boolean	Build_Viewport_Transformation(ViewportPtr);
extern void		Viewport_Init(ViewportPtr);
extern void		Create_View_Menu(Widget, Boolean, WindowInfoPtr);
extern void		Create_Window_Menu(Widget, WindowInfoPtr);
extern void		Window_Set_Draw_Mode(WindowInfoPtr, int);
extern void		Window_Draw_Mode_Callback(Widget, XtPointer, XtPointer);

/*
**	Functions from conversions.c.
*/
extern void Convert_World_To_View(Vector*, Vertex*, short, Viewport*);
extern void Convert_View_To_Screen(Vertex*, short, Viewport*, short, short,
								   double);
extern void	Convert_Plane_World_To_View(FeaturePtr, Viewport*, FeaturePtr);
extern void	Convert_Line_World_To_View(FeaturePtr, Viewport*, FeaturePtr);


/*
**	Functions for wireframe manipulation.
*/
extern WireframePtr			Wireframe_Copy(WireframePtr);
extern WireframePtr			Object_To_Wireframe(ObjectInstancePtr, Boolean);
extern void					Wireframe_Destroy(WireframePtr);
extern struct _CSGWireframe	*Wireframe_To_CSG(WireframePtr, Boolean);
extern WireframePtr			CSG_To_Wireframe(struct _CSGWireframe*);
extern WireframePtr			Wireframe_Simplify(WireframePtr);
extern WireframePtr			Face_Triangulate(WireframePtr, FacePtr);
extern Boolean				Wireframe_Has_Attributes(WireframePtr);
extern int					Wireframe_Count_Edges(WireframePtr);


/*
**	Functions from utils.c.
*/
extern char 		*EMalloc(unsigned int);
extern char 		*WMalloc(unsigned int);
extern char 		*ERealloc(char*, unsigned int);
extern char 		*WRealloc(char*, unsigned int);
extern void 		Popup_Error(char*, Widget, char*);
extern Dimension	Match_Widths(Widget*, int);
extern void 		Destroy_World(Boolean);
extern Boolean		Check_Rectangle_Intersection(XPoint, XPoint, XPoint,XPoint);
extern Vector		Map_Point_Onto_Plane(XPoint, FeatureData, Viewport*, short,
									short, int);
extern Vector		Map_Point_Onto_Line(XPoint, FeatureData, Viewport*, short,
									short, int);
extern void			 Set_Prompt(WindowInfoPtr, String);
extern double		Round_To_Snap(double, double);
extern Boolean		Points_Colinear(Vector, Vector, Vector);
extern void			Set_WindowInfo(WindowInfo*);
extern Vector		Extract_Euler_Angles(Matrix);
extern void			Copy_Objects_Callback(Widget, XtPointer, XtPointer);
extern void			Save_Temp_Filename(char*);
extern void			Vector_To_Rotation_Matrix(Vector*, Matrix*);

/*
**	Functions from base_objects.c.
*/
extern Boolean			Initialize_Base_Objects();
extern BaseObjectPtr	Get_Base_Object_From_Label(String);
extern Boolean			Add_Instance_To_Base(ObjectInstancePtr, BaseObjectPtr);
extern void				Remove_Instance_From_Base(ObjectInstancePtr);
extern void				Destroy_All_Base_Objects();
extern BaseObjectPtr	Add_CSG_Base_Object(struct _CSGNode*, char*,
										WireframePtr, WireframePtr);
extern void				CSG_Destroy_Base_Object(Widget, BaseObjectPtr, Boolean);
extern void				CSG_Copy_Base_Object(Widget, BaseObjectPtr);
extern BaseObjectPtr	Add_Wireframe_Base_Object(char*, WireframePtr);
extern void				Wireframe_Destroy_Base_Object(Widget, BaseObjectPtr);
extern void				Base_Change(ObjectInstancePtr, BaseObjectPtr, Boolean,
									WindowInfoPtr);
extern void				Base_Change_List(WindowInfoPtr, InstanceList,
										 BaseObjectPtr);
extern void				Base_Change_Callback(Widget, XtPointer, XtPointer);
extern void				Base_Change_Select_Callback(Widget, BaseObjectPtr);
/* Located in csg_reference.c. */
extern void				Select_Base_Reference(BaseObjectPtr);


/*
**	Functions from instances.c.
*/
extern ObjectInstancePtr	Create_Instance(BaseObjectPtr, String);
extern ObjectInstancePtr	Copy_Object_Instance(ObjectInstancePtr);
extern void		Rename_Instance(ObjectInstancePtr, char*);
extern int		Transform_Instance(ObjectInstancePtr, Transformation*, Boolean);
extern void		Displace_Instance(ObjectInstancePtr, Vector);
extern void		Modify_Instance_Attributes(ObjectInstancePtr, Attributes*, int);
extern void		Destroy_Instance(ObjectInstancePtr);


/*  Functions from instance_list.c
*/
extern InstanceList Find_Object_In_Instances(ObjectInstancePtr, InstanceList);


/*
**	Functions from bounds.c
*/
extern Cuboid	Calculate_Bounds(Vector*, int);
extern Extent2D	Calculate_Projection_Extents(Vertex*, int);
extern void		Update_Projection_Extents(InstanceList);
extern Cuboid	Transform_Bound(Cuboid*, Transformation*);


/*
**	Functions from draw.c
*/
extern void View_Update(WindowInfoPtr, InstanceList, int);
extern void	Draw_Initialize();
extern void Draw_Edges(Display*, Drawable, GC, GC, WireframePtr, Vertex*,
						Vector*, Vector*, Viewport*);
extern void Draw_Visible_Edges(Display*, Drawable, GC, WireframePtr,
						Vertex*, Vector*, Vector*, Viewport*);
extern void Draw_All_Edges(Display*, Drawable, GC, WireframePtr, Vertex*,
						   ViewportPtr);
extern void Draw_Visible_Edges_XOR(Display*, Drawable, GC, WireframePtr,
						Vertex*, Vector*, Vector*, Viewport*);
extern void Draw_All_Edges_XOR(Display*, Drawable, GC, WireframePtr, Vertex*,
							   ViewportPtr);


/*
**	Functions from new_object.c
*/
extern void New_Object_Popup_Callback(Widget, XtPointer, XtPointer);
extern void	Select_Object_Popup(WindowInfoPtr, int);
extern void Add_Object_To_World(ObjectInstancePtr, Boolean);
extern ObjectInstancePtr	Create_New_Object_From_Base(WindowInfoPtr,
														BaseObjectPtr, Boolean);


/*
**	From dense_wireframe.c, wireframe changing functions.
*/
extern int	Wireframe_Density_Level(ObjectInstancePtr);
extern void	Wireframe_Denser_Callback(Widget, XtPointer, XtPointer);
extern void	Wireframe_Thinner_Callback(Widget, XtPointer, XtPointer);
extern void	Object_Change_Wire_Level(ObjectInstancePtr, int level);
extern void	Object_Change_Wire(ObjectInstancePtr);
extern int	Edge_Add_Edge(EdgePtr*, int, int, int);


/*
**	Functions for creating lights.
*/
extern void	Create_Light_Callback(Widget, XtPointer, XtPointer);
extern void	Create_Spotlight_Callback(Widget, XtPointer, XtPointer);
extern void	Create_Arealight_Callback(Widget, XtPointer, XtPointer);
extern void Ambient_Light_Callback(Widget, XtPointer, XtPointer);
extern void	Ambient_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	Light_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	Set_Light_Attributes(InstanceList, Boolean, Boolean);
extern void	Set_Spotlight_Attributes(InstanceList, Boolean);
extern void	Set_Arealight_Attributes(InstanceList);

/*
**	Functions from main_view.c
*/
extern void Sensitize_Main_Buttons(Boolean);


/*
**	Functions from main_view.c
*/
extern void Edit_Update_Object_Dependents(ObjectInstancePtr);


/*	Menu functions. */
extern void	Create_Edit_Menu(WindowInfoPtr);
extern void	Edit_Objects_Function(Widget, XtPointer, XtPointer);
extern void	Add_Instance_To_Edit(WindowInfoPtr, InstanceList, Boolean);
extern void	Delete_Edit_Instance(WindowInfoPtr, InstanceList);


/*
**	Edit functions.
*/
extern void	Edit_Instance(WindowInfoPtr, InstanceList);
extern void	Edit_Set_Cursor_Action(Widget, XEvent*, String*, Cardinal*);
extern void	Edit_Start_Drag(Widget, XEvent*, String*, Cardinal*);
extern void	Edit_Shell_Finished(Widget,XtPointer,XtPointer);
extern void	Edit_Continue_Drag(Widget, XEvent*, String*, Cardinal*);
extern void	Edit_Finish_Drag(Widget, XEvent*, String*, Cardinal*);
extern void	Edit_Sensitize_Buttons(Boolean, Boolean);
extern void	Add_Name_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	Edit_Remove_Dependencies(FeaturePtr, ObjectInstancePtr);
extern void	Constraint_Remove_References(ObjectInstancePtr, ObjectInstancePtr);
extern void	Constraint_Manipulate_Specs(FeaturePtr, ObjectInstancePtr, void*,
										int, SpecFunction);
extern void	Edit_Remove_Obj_From_Dependencies(FeatureSpecPtr, ObjectInstancePtr,
											  void*, int);
extern void	Maintain_Toggle_Callback(Widget, XtPointer, XtPointer);

/*
**	Viewport changing functions.
*/
extern void Initiate_Viewfrom_Change(Widget, XtPointer, XtPointer);
extern void	Initiate_Pan_Change(Widget, XtPointer, XtPointer);
extern void Initiate_Distance_Change(WindowInfoPtr, Boolean);
extern void Start_Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Stop_Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Start_Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern void Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern void Stop_Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern void	Change_Lookat_Point(int*, FeatureSpecType*);
extern void	Change_Lookup_Vector(int*, FeatureSpecType*);

/*
**	Viewport saving and recalling.
*/
extern void	View_Save_Current_Callback(Widget, XtPointer, XtPointer);
extern void	View_Recall_Callback(Widget, XtPointer, XtPointer);
extern void	View_Name_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	View_Save(Viewport*, char*);
extern void	View_Delete_Callback(Widget, XtPointer, XtPointer);
extern void	View_Reset();

/*	from events.c */
extern void	Redraw_Main_View(Widget, XtPointer, XtPointer);
extern void	Cancel_Viewport_Change();
extern WindowInfoPtr	Get_Active_Window();
extern void	Change_Lookat_Callback(Widget, XtPointer, XtPointer);
extern void	Change_Lookup_Callback(Widget, XtPointer, XtPointer);


/*
**	Camera related functions.
*/
extern void Camera_To_Viewport(Camera*, ViewportPtr);
extern void Camera_To_Window(WindowInfoPtr);
extern void	Viewport_To_Camera(ViewportPtr, Widget, Camera*, Boolean);


/*
**	Object selection functions.
*/
extern void Start_Selection_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Continue_Selection_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Finish_Selection_Drag(Widget, XEvent*, String*, Cardinal*);


/*
**	Load functions.
*/
extern void Load_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Merge_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Load_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern FILE	*Open_Load_File_Name(char**);
extern void Load_File(FILE*, Boolean);

/*
**	Save callback function.
*/
extern void Save_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Save_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	Save_Func(FILE*);
extern int Save_Frame(FILE *,int);


/*
**	Export functions.
*/
extern int  Export_File(FILE*, char*, ScenePtr, Boolean);
extern void Export_Callback(Widget, XtPointer, XtPointer);
extern void Export_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*	Zoom action. */
extern void Zoom_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Zoom_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*
**	Quit function.
*/
extern void Quit_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Quit_Func(Widget, XtPointer, XtPointer);


/*
**	Delete function
*/
extern void Delete_Objects_Callback(Widget, XtPointer, XtPointer);


/*
**	misc functions.
*/
extern void Reset_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Clear_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Image_Size_Callback(Widget, XtPointer, XtPointer);

/*
**	Callback for rename button in rename dialog.
*/
extern void Rename_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*
**	Layers callbacks and actions.
*/
extern void	New_Layer_Action_Function(Widget, XEvent*, String*, Cardinal*);
extern void	Merge_Layer_Action_Function(Widget, XEvent*, String*, Cardinal*);

/*
**	Functions to invoke the apply button at the bottom of the screen.
*/
extern void Apply_Button_Callback(Widget, XtPointer, XtPointer);
extern void Apply_Button_Action(Widget, XEvent*, String*, Cardinal*);

/*
**	Functions for creating/editing CSG objects.
*/
extern void	Sensitize_CSG_Buttons(Boolean);
extern void	Set_CSG_Related_Sensitivity(Boolean);
extern void	CSG_Window_Popup(Widget, XtPointer, XtPointer);
extern void	New_CSG_Instance(ObjectInstancePtr);
extern void	Edit_CSG_Object(ObjectInstancePtr);
extern void	CSG_Tree_Notify_Func(Widget, XEvent*, String*, Cardinal*);
extern void	CSG_Menu_Button_Up_Func(Widget, XEvent*, String*, Cardinal*);
extern void	CSG_Tree_Motion_Func(Widget, XEvent*, String*, Cardinal*);
extern void	CSG_Complete_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void	CSG_Reset();


/* Wireframe functions. For importing, exporting wireframes. */
extern void	Wireframe_Load_Callback(Widget, XtPointer, XtPointer);
extern void	Wireframe_Delete_Callback(Widget, XtPointer, XtPointer);
extern void	Wireframe_Select_Popup(int);
extern void	Wireframe_Add_Select_Option(BaseObjectPtr);
extern void	Wireframe_Select_Destroy_Widget(int);
extern void	Set_Wireframe_Related_Sensitivity(Boolean);
extern void	OFF_Save_Wireframe(Widget, BaseObjectPtr);


/* Attributes functions. */
extern void	Attributes_Change_String(InstanceList, char*, Boolean, Boolean);
extern void	Set_Attributes_Callback(Widget, XtPointer, XtPointer);
extern int	Save_Declarations(FILE*);
extern void	Add_Declarations(char*);
extern void	Clear_Declarations();
extern void	Specific_Attributes_Callback();

/*
**	Preview functions.
*/
extern void	Preview_Callback(Widget, XtPointer, XtPointer);
extern void	Perform_Preview(WindowInfoPtr, Raytracer, InstanceList, int, int);
extern void	Preview_Sensitize(Boolean);

/*
**	POV functions
*/
extern int	POV_Save_Includes(FILE*);
extern void	POV_Add_Include(char*);
extern void	POV_Clear_Includes();
extern void	POV_Includes_Callback(Widget, XtPointer, XtPointer);

/*
**	Action for renderman dialog.
*/
extern void	Renderman_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*
**	One function from the SelFile code.
*/
extern void	SFpositionWidget(Widget);

/*
**  copy_obj.c functions
*/
extern InstanceList Copy_InstanceList(InstanceList, InstanceList);
extern ObjectInstancePtr Copy_ObjectInstance(ObjectInstancePtr old);
extern DependentList Copy_DependentList(DependentList old, short num);
extern void Copy_FeatureData(FeatureData *old, FeatureData *new);
extern FeaturePtr Copy_FeaturePtr(FeaturePtr old, short num);
extern InstanceList Copy_InstancePtrList(InstanceList list, InstanceList all);
extern Vector *Copy_Vector(Vector *v1, int n);
extern Matrix *Copy_Matrix(Matrix *m1, int n);
extern Vertex *Copy_Vertex(Vertex *m1, int n);
extern Boolean *Copy_Boolean(Boolean *m1, int n);
extern Wireframe *Copy_Wireframe(Wireframe *old);
extern FacePtr Copy_Face(FacePtr old, int num);
extern AttributePtr *Copy_AttributePtr(AttributePtr *old, int num);
extern AttributePtr Copy_Attribute(AttributePtr old);
extern LightInfo *Copy_LightInfo(LightInfo *old);
extern EdgePtr Copy_Edge(EdgePtr f);
extern ObjectInstancePtr Object_From_Label(InstanceList, InstanceList, String);
extern int Num_RefFeatures(FeatureType type);
extern void Update_FeaturePtrs(InstanceList list, InstanceList otherlist);
extern void Update_DependentList(InstanceList list, InstanceList otherlist);
extern void Update_FeatureSpec(InstanceList, InstanceList, FeaturePtr);
extern void Reset_BaseObjectInstances(BaseObjectPtr *base, InstanceList list);
extern void Free_InstanceList(InstanceList freelist);
extern void Free_FeatureLabel(FeaturePtr f, int num);
extern void Free_Deps(ObjectInstance *);

/*
**  print.c functions
*/
extern void Print_XPoint(XPoint pt);
extern void Print_Instance(ObjectInstancePtr elmt);
extern void Print_InstanceList(InstanceList list);
extern void Print_Vector(Vector *v);
extern void Print_Matrix(Matrix *m);
extern void Print_KeyFrame(KeyFrame *kf);
extern void Print_Quaternion(Quaternion *q);
extern void Print_FeatureType(FeatureType *f);
extern void Print_FeatureSpec(FeatureSpecifier *f);
extern void Print_FeatureSpecType(FeatureSpecType *f);
extern void Print_FeatureData(FeatureData *f);
extern void Print_ScaleCons(ObjectInstance *obj);
extern void Print_SelObjectScale(void);
extern void	Maintenance(Widget, XEvent*, String*, Cardinal*);
extern char *Get_String(char *,char *);
extern void Get_String_Return(Widget,XEvent *,String *,Cardinal *);
extern void Print_Flags(int flags);


/* I put this proto here, although the source is in rotate.c. */
extern Matrix Quaternion_To_Matrix(Quaternion);


/*
**  keyframe.c functions
*/
extern void Next_KeyFrame(Widget,XEvent *,String *,Cardinal *);
extern void Prev_KeyFrame(Widget,XEvent *,String *,Cardinal *);
extern Boolean New_KeyFrame(int,Boolean);
extern void Clone_KeyFrame(Widget,XEvent *,String *,Cardinal *);
extern void Remove_KeyFrame(Widget,XEvent *,String *,Cardinal *);
extern void Update_to_KeyFrame(KeyFrame *);
extern void Update_from_KeyFrame(KeyFrame *,Boolean);
extern void Renumber_KeyFrames(KeyFrameList);
extern KeyFrame * Nth_KeyFrame(int, KeyFrameList);
extern KeyFrame * Last_KeyFrame(KeyFrameList);
extern int Count_KeyFrames(KeyFrameList);
extern int Count_Frames(KeyFrameList);
extern void Initialize_KeyFrames(KeyFrameList *);
extern void PumpUp_All(Widget,XEvent *,String *,Cardinal *);
extern void PumpDown_All(Widget,XEvent *,String *,Cardinal *);
extern void PumpUp_Next(Widget,XEvent *,String *,Cardinal *);
extern void PumpDown_Next(Widget,XEvent *,String *,Cardinal *);
extern void Clone_to_Current(Widget,XEvent *,String *,Cardinal *);
extern void Clone_to_Next(Widget,XEvent *,String *,Cardinal *);
extern void Clone_to_Prev(Widget,XEvent *,String *,Cardinal *);
extern void Clone_to_Keyframes(Widget,XEvent *,String *,Cardinal *);
extern void DeleteObj_from_KeyFrame(KeyFrame *, char *);
extern void Zero_Next(Widget,XEvent *,String *,Cardinal *);
extern void Zero_All(Widget,XEvent *,String *,Cardinal *);
extern void Pump_All(Widget,XEvent *,String *,Cardinal *);
extern void Clone_Attributes(Widget,XEvent *,String *,Cardinal *);
extern void Synch_View(Widget,XEvent *,String *,Cardinal *);

extern void Delete_from_Keyframes(Widget,XEvent *,String *,Cardinal *);

/*
** animate.c
*/
extern Vector *Vector_Spline(int,int,double*,double*,double*,double*);
extern Quaternion 
	*Quaternion_Spline(int,int,double*,double*,double*,double*,double*);
extern int Seq_Start(Boolean *o_inframe, int seq, int nf);
extern int Seq_Stop(Boolean *o_inframe, int seq, int nf);
extern int Numkf_in_Seq(KeyFrame *key_frames, int start, int stop);
extern void Spline_Object(ObjectInstance *obj);
extern void Spline_Viewport(KeyFrame *key_frames);
extern void Spline_World(void);
extern void Constrain_World(void);
extern void Update_from_Frame(int i,InstanceList);
extern void Update_to_Frame(int i,InstanceList);
extern int Frame_to_Seq(ObjectInstance *obj, int frame);
extern int Frame_to_Seqf(ObjectInstance *obj, int frame);
extern void Animate(Widget w, XEvent *e, String *s, Cardinal *num);
extern void Export_Animation(Widget w, XEvent *e, String *s, Cardinal *num);
extern char *Render_Command(Raytracer,char*,char*,int,int);


/*
** my_misc.c
*/
extern Quaternion QMul(Quaternion,Quaternion);
extern Matrix Rot_To_Matrix(double,double,double);
extern Matrix Scale_To_Matrix(double,double,double);
extern Matrix Quat_To_Matrix(Quaternion);
extern Quaternion AngleVector_To_Quaternion(double,Vector);
extern void Edit_Accel(Widget,XEvent*,String*,Cardinal*);
extern void unEdit_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Viewfrom_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Apply_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Recall_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Zoom_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Pan_Accel(Widget,XEvent*,String*,Cardinal*);
extern void LA_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Quit_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Help_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Close_Accel(Widget,XEvent*,String*,Cardinal*);
extern void QQuit_Accel(Widget,XEvent*,String*,Cardinal*);
extern void New_Accel(Widget,XEvent*,String*,Cardinal*);
extern void Free_LabelMap(LabelMapPtr map);
extern String Find_LabelMap(LabelMapPtr map, String label);
extern LabelMapPtr Add_LabelMap(LabelMapPtr map, String old, String new);
extern Boolean ObjectLabel_Exists(char *,int);


/*
**  spline.c functions
*/
extern double *curv0(double *, double *, int, int, double);
extern double *curvpp(double *, double *, int);
extern double *curvpp2(double *, double *, double *, int);
extern void curv1(double *, double *, double *, int, double);
extern double curv2(double *,double *,double *,double, int, double);


/*
**	ELK evaluation function, if required.
*/
#if ELK_SUPPORT
extern void	Elk_Eval_String(Widget, XEvent*, String*, Cardinal*);
#endif /* ELK_SUPPORT */

#endif /* __SCED_PROTO__ */
