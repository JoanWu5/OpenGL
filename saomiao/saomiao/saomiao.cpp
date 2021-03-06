#include "pch.h"
#include <GL/glut.h> 
#include <stdlib.h> 
#include <stdio.h>  
#include <windows.h>  
#define NULL 0   
#define WINDOW_HEIGHT 50   //定义窗口高为400 
#define WINDOW_WIDTH 50    //定义窗口宽为400 

struct dcPt { //dcPt是一个点的结构体，表示像素点 
	int x;
	int y;
};

void setPixel(GLint x, GLint y)  //用OpenGL函数改写setPixel(创建像素点) 
{
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}
/*定义边表结点*/
typedef struct tEdge {
	int yUpper;
	float xIntersect, dxPerScan;
	struct tEdge * next;
} Edge;

//======================================================================== 
/* 向边表中添加边Inserts edge into list in order to increase xIntersect field. */
void insertEdge(Edge * list, Edge * edge)
{
	Edge * p, *q = list;
	p = q->next;
	while (p != NULL) {
		if (edge->xIntersect < p->xIntersect)
			p = NULL;

		else {
			q = p;
			p = p->next;
		}
	}
	edge->next = q->next;
	q->next = edge;
}
//---------------------------------------------------------------------------------- 
/* For an index, return y-coordinate of next nonhorizontal line */
int yNext(int k, int cnt, dcPt * pts)
{
	int j;

	if ((k + 1) > (cnt - 1))
		j = 0;
	else
		j = k + 1;
	while (pts[k].y == pts[j].y)
		if ((j + 1) > (cnt - 1))
			j = 0;
		else
			j++;
	return (pts[j].y);
}
//--------------------------------------------------------------------------------------- 
/* Store lower-y coordinate and inverse slope for each edge.  Adjust and store upper-y coordinate for edges that are the lower member of a monotically increasing or decreasing pair of edges */
/*存储边的Y值及斜率倒数,给边节点赋值*/
void makeEdgeRec(dcPt lower, dcPt upper, int yComp, Edge * 	edge, Edge * edges[])
{
	edge->dxPerScan =
		(float)(upper.x - lower.x) / (upper.y - lower.y);
	edge->xIntersect = lower.x;
	if (upper.y < yComp)
		edge->yUpper = upper.y - 1;
	else
		edge->yUpper = upper.y;
	insertEdge(edges[lower.y], edge);
}
//---------------------------------------------------------------------------------- 
/*建立边表*/
/*入口参数 cnt:多边形定点数
		   pts:顶点坐标数组指针
		   edges[]*/
void buildEdgeList(int cnt, dcPt * pts, Edge * edges[])
{
	Edge * edge;
	dcPt v1, v2;
	int i, yPrev = pts[cnt - 2].y;

	v1.x = pts[cnt - 1].x; v1.y = pts[cnt - 1].y;
	for (i = 0; i < cnt; i++) {
		v2 = pts[i];
		if (v1.y != v2.y) {                /* nonhorizontal line 非水平边*/
			edge = (Edge *)malloc(sizeof(Edge));
			if (v1.y < v2.y)                 /* up-going edge 即将加入的边     */
				makeEdgeRec(v1, v2, yNext(i, cnt, pts), edge, edges);
			else                             /* down-going edge 即将扫描完的边   */
				makeEdgeRec(v2, v1, yPrev, edge, edges);
		}
		yPrev = v1.y;
		v1 = v2;
	}
}
//---------------------------------------------------------------------------------- 
/*建立活动边表*/
void buildActiveList(int scan, Edge * active, Edge * edges[])
{
	Edge * p, *q;

	p = edges[scan]->next;
	while (p) {
		q = p->next;
		insertEdge(active, p);
		p = q;
	}
}
//----------------------------------------------------------------------------------- 
/*扫描*/
/*被函数scanFill (int cnt, dcPt * pts)调用*/
void fillScan(int scan, Edge * active)
{
	Edge * p1, *p2;
	int i;

	p1 = active->next;
	while (p1) {
		p2 = p1->next;
		for (i = p1->xIntersect; i < p2->xIntersect; i++)
			setPixel((int)i, scan);
		p1 = p2->next;
	}
}
//----------------------------------------------------------------------------------- 
/*将边从链表中删除*/
/*被函数updateActiveList (int scan, Edge * active) 调用*/
void deleteAfter(Edge * q)
{
	Edge * p = q->next;

	q->next = p->next;
	free(p);
}

/* 将扫描完毕的边从活动边表中删除，更新活动边表结点中的xIntersect */
void updateActiveList(int scan, Edge * active)
{
	Edge * q = active, *p = active->next;

	while (p)
		if (scan >= p->yUpper) {
			p = p->next;
			deleteAfter(q);
		}
		else {
			p->xIntersect = p->xIntersect + p->dxPerScan;
			q = p;
			p = p->next;
		}
}
//------------------------------------------------------------------------------------- 
/*更新活动边表，将新的边加入活动边表中*/
void resortActiveList(Edge * active)
{
	Edge * q, *p = active->next;

	active->next = NULL;
	while (p) {
		q = p->next;
		insertEdge(active, p);
		p = q;
	}
}
//------------------------------------------------------------------------------------------ 
/*填充*/
void scanFill(int cnt, dcPt * pts)
{
	Edge *edges[WINDOW_HEIGHT], *active;
	int i, scan;

	for (i = 0; i < WINDOW_HEIGHT; i++) {
		edges[i] = (Edge *)malloc(sizeof(Edge));
		edges[i]->next = NULL;
	}
	buildEdgeList(cnt, pts, edges);//入口参数都是给这个函数用的 
	active = (Edge *)malloc(sizeof(Edge));
	active->next = NULL;

	for (scan = 0; scan < WINDOW_HEIGHT; scan++) {
		buildActiveList(scan, active, edges);
		if (active->next) {
			fillScan(scan, active);
			updateActiveList(scan, active);
			resortActiveList(active);
			Sleep(20); //为了放慢填充速度，便于观看填充过程，每填充一行停顿10毫秒，Sleep函数包含在头文件windows.h里面 
		}
	}
}
//------------------------------------------------------------------------------------ 
/*OpenGL 初始化*/
void init(void)
{
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
}
/*定义顶点坐标*/
void myDraw()
{
	dcPt pts[] = { //pts是表示填充图元的顶点数组 

	/*50, 50,
	300, 20,
	300, 300,
	200,100,
	150,350,
	20,120 */ /*不规则多边形，有凹陷凸出*/
		2,2,
		2,4,
		8,6,
		12,2,
		8,1,
		6,2
//*领结形状*/ 
	};

	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 0.0, 0.0);
	scanFill(6, pts);// 第一个参数为填充图元的顶点数，第二个参数为顶点坐标数组// 
	glFlush();
}
/*主函数*/
void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(50, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("扫描线填充算法");

	init();
	glutDisplayFunc(myDraw); //绘图循环进行，如果在mydraw函数中添加输入与输出控制多边形的顶点数与顶点坐标，则需要不断在控制台进行输入，有问题。 
	glutMainLoop(); //让整个绘图循环进行    
}