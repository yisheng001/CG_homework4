//   Copyright © 2021, Renjie Chen @ USTC

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#define FREEGLUT_STATIC
#include "gl_core_3_3.h"
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#define TW_STATIC
#include <AntTweakBar.h>

#include <vector>
#include <string>

#include "glprogram.h"
#include "MyImage.h"
#include "VAOImage.h"
#include "VAOMesh.h"
#include"math.h"

using namespace std;

GLProgram MyMesh::prog;

MyMesh M;
int viewport[4] = { 0, 0, 1280, 960 };

bool showATB = true;

std::string imagefile = "boy.png";

MyImage img;
int resize_width, resize_height;

int mousePressButton;
int mouseButtonDown;
int mousePos[2];

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, viewport[2], viewport[3]);
	M.draw(viewport);

	if (showATB) TwDraw();
	glutSwapBuffers();
}

void onKeyboard(unsigned char code, int x, int y)
{
	if (!TwEventKeyboardGLUT(code, x, y)) {
		switch (code) {
		case 17:
			exit(0);
		case 'f':
			glutFullScreenToggle();
			break;
		case ' ':
			showATB = !showATB;
			break;
		}
	}

	glutPostRedisplay();
}

void onMouseButton(int button, int updown, int x, int y)
{
	if (!showATB || !TwEventMouseButtonGLUT(button, updown, x, y)) {
		mousePressButton = button;
		mouseButtonDown = updown;

		mousePos[0] = x;
		mousePos[1] = y;
	}

	glutPostRedisplay();
}

void onMouseMove(int x, int y)
{
	if (!showATB || !TwEventMouseMotionGLUT(x, y)) {
		if (mouseButtonDown == GLUT_DOWN) {
			if (mousePressButton == GLUT_MIDDLE_BUTTON) {
				M.moveInScreen(mousePos[0], mousePos[1], x, y, viewport);
			}
		}
	}

	mousePos[0] = x; mousePos[1] = y;

	glutPostRedisplay();
}

void onMouseWheel(int wheel_number, int direction, int x, int y)
{
	if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
	}
	else
		M.mMeshScale *= direction > 0 ? 1.1f : 0.9f;

	glutPostRedisplay();
}

int initGL(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(960, 960);
	glutInitWindowPosition(200, 50);
	glutCreateWindow(argv[0]);

	// !Load the OpenGL functions. after the opengl context has been created
	if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
		return -1;

	glClearColor(1.f, 1.f, 1.f, 0.f);

	glutReshapeFunc([](int w, int h) { viewport[2] = w; viewport[3] = h; TwWindowSize(w, h); });
	glutDisplayFunc(display);
	glutKeyboardFunc(onKeyboard);
	glutMouseFunc(onMouseButton);
	glutMotionFunc(onMouseMove);
	glutMouseWheelFunc(onMouseWheel);
	glutCloseFunc([]() {exit(0); });
	return 0;
}

void uploadImage(const MyImage& img)
{
	int w = img.width();
	int h = img.height();
	float x[] = { 0, 0, 0, w, 0, 0, w, h, 0, 0, h, 0 };
	M.upload(x, 4, nullptr, 0, nullptr);

	M.tex.setImage(img);
	M.tex.setClamping(GL_CLAMP_TO_EDGE);
}

void readImage(const std::string& file)
{
	int w0 = img.width(), h0 = img.height();
	img = MyImage(file);
	uploadImage(img);
	resize_width = img.width();
	resize_height = img.height();

	if (w0 != img.width() || h0 != img.height()) M.updateBBox();
}

//添加函数
inline std::vector<std::vector<double>>dx(std::vector<BYTE>data, int w, int h, int comp)
{
	std::vector<double>data0(w * comp * h);
	int i, j, k;
	for (i = 0; i < h; i++)//先求第一列的导数
	{
		for (k = 0; k < comp; k++)
		{
			data0[i * w * comp + k] = data[i * w * comp + k + 1 * comp] - data[i * w * comp + k];
		}
	}
	for (j = 1; j < w - 1; j++)//从第2列到第w-1列
	{
		for (i = 0; i < h; i++)
		{
			for (k = 0; k < comp; k++)
			{
				data0[(i * w + j) * comp + k] = data[(i * w + j + 1) * comp + k] - data[(i * w + j - 1) * comp + k];
			}
		}
	}
	for (i = 0; i < h; i++)//求最后一列的导数
	{
		for (k = 0; k < comp; k++)
		{
			data0[i * w * comp + k + (w - 1) * comp] = data[i * w * comp + k + (w - 1) * comp] - data[i * w * comp + k + (w - 2) * comp];
		}
	}
	std::vector<std::vector<double>>S(h, std::vector<double>(w));
	double S1;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			S[i][j] = 0;
			for (k = 0; k < comp; k++)
			{
				if (data0[(i * w + j) * comp + k] > 0)
				{
					S[i][j] = S[i][j] + data0[(i * w + j) * comp + k];//S1 = S1 + data0[(i * w + j) * comp + k] * data0[(i * w + j) * comp + k];
				}
				else
				{
					S[i][j] = S[i][j] - data0[(i * w + j) * comp + k];
				}
				S[i][j] = S[i][j] / 3;
			}
			/*S1 = 0;
			for (k = 0; k < comp; k++)
			{
				S1 = S1 + data0[(i * w + j) * comp + k] * data0[(i * w + j) * comp + k];
			}
			S1 = sqrt(S1);
			S[i][j] = S1;*/
		}
	}
	return S;
}

vector<vector<double>>dy(vector<BYTE>data, int w, int h, int comp)
{
	std::vector<BYTE>data0(w * comp * h);
	int i, j, k;
	for (i = 0; i < w; i++)//先求第一行的导数
	{
		for (k = 0; k < comp; k++)
		{
			data0[i * comp + k] = data[(i + w) * comp + k] - data[i * comp + k];
		}
	}
	for (i = 1; i < h - 1; i++)//从第2行到第w-1行
	{
		for (j = 0; j < w; j++)
		{
			for (k = 0; k < comp; k++)
			{
				data0[(i * w + j) * comp + k] = data[((i + 1) * w + j) * comp + k] - data[((i - 1) * w + j) * comp + k];
			}
		}
	}
	for (i = 0; i < w; i++)//求最后一行的导数
	{
		for (k = 0; k < comp; k++)
		{
			data0[(h - 1) * w * comp + k + i * comp] = data[(h - 1) * w * comp + k + i * comp] - data[(h - 2) * w * comp + k + i * comp];
		}
	}
	std::vector<std::vector<double>>S(h, std::vector<double>(w));
	double S1;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			S[i][j] = 0;
			for (k = 0; k < comp; k++)
			{
				if (data0[(i * w + j) * comp + k] > 0)
				{
					S[i][j] = S[i][j] + data0[(i * w + j) * comp + k];//S1 = S1 + data0[(i * w + j) * comp + k] * data0[(i * w + j) * comp + k];
				}
				else
				{
					S[i][j] = S[i][j] - data0[(i * w + j) * comp + k];
				}
				S[i][j] = S[i][j] / 3;
			}
		}
	}
	return S;
}

int min(vector<double>data, int n)
{
	int i, j;
	i = 0;
	for (j = 0; j < n; j++)
	{
		if (data[j] < data[i])
		{
			i = j;
		}
	}
	return i;
}

//从上向下计算能量
vector<int>calculateEnergy_rows(vector<BYTE>data, int h, int w, int comp)
{
	vector<vector<double>>data0(h, vector<double>(w)), data_dx(h, vector<double>(w)), data_dy(h, vector<double>(w));
	vector<vector<double>>data1(h, vector<double>(w));
	vector<double>k(3);
	vector<vector<int>>data_trace(h, vector<int>(w));
	//data0存各个点的能量；data1存从上向下所在点的累计最小能量；
	//data_trace存从上一行的左上（0），正上（1），右上（2）下来
	data_dx = dx(data, w, h, comp);
	data_dy = dy(data, w, h, comp);
	int i, j, index;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			data1[i][j] = data_dx[i][j] + data_dy[i][j];
			data0[i][j] = data_dx[i][j] + data_dy[i][j];
		}
	}
	for (i = 1; i < h; i++)
	{
		if (data1[i - 1][0] < data1[i - 1][1])//计算第一列的能量
		{
			data1[i][0] = data1[i][0] + data1[i - 1][0];
			data_trace[i][0] = 1;//从正上下来
		}
		else
		{
			data1[i][0] = data1[i][0] + data1[i - 1][1];
			data_trace[i][0] = 2;//从右上下来
		}
		for (j = 1; j < w - 1; j++)
		{
			k[0] = data1[i - 1][j - 1];
			k[1] = data1[i - 1][j];
			k[2] = data1[i - 1][j + 1];
			index = 0;
			if (k[1] < k[0])
			{
				index = 1;
			}
			if (k[2] < k[index])
			{
				index = 2;
			}
			data1[i][j] = data1[i - 1][j - 1 + index] + data1[i][j];
			data_trace[i][j] = index;
		}
		if (data1[i - 1][w - 2] < data1[i - 1][w - 1])//计算第w列的能量
		{
			data1[i][w - 1] = data1[i][w - 1] + data1[i - 1][w - 2];
			data_trace[i][w - 1] = 0;
		}
		else
		{
			data1[i][w - 1] = data1[i][w - 1] + data1[i - 1][w - 1];
			data_trace[i][w - 1] = 1;
		}
	}
	index = min(data1[h - 1], w);
	vector<int>Trace(h);
	Trace[h - 1] = index;
	for (i = h - 2; i >= 0; i--)
	{
		index = data_trace[i + 1][index] - 1 + index;
		Trace[i] = index;
	}
	return Trace;
}

//删除一列
vector<BYTE>delete_cols(vector<BYTE>data, int h, int w, int comp, vector<int>Trace)
{
	vector<BYTE>data1(h * (w - 1) * comp);
	vector<int> data0(h * w * comp);
	int i, j, k;
	for (i = 0; i < h * w * comp; i++)
	{
		data0[i] = data[i];
	}
	for (i = 0; i < h; i++)
	{
		j = Trace[i];
		for (k = 0; k < comp; k++)
		{
			data0[(i * w + j - 1) * comp + k] = (data0[(i * w + j - 1) * comp + k] + data0[(i * w + j) * comp + k]) / 2;
			data0[(i * w + j + 1) * comp + k] = (data0[(i * w + j + 1) * comp + k] + data0[(i * w + j) * comp + k]) / 2;
			data0[(i * w + j) * comp + k] = -1;//将要删除的像素点的四个量标记为-1
		}
	}
	j = 0;
	for (i = 0; i < h * (w - 1) * comp; i++)
	{
		while (data0[j] < 0)
		{
			j++;
		}
		data1[i] = data0[j];
		j++;
	}
	return data1;
}

MyImage seamCarving(const MyImage& img, int w, int h)
{
	// TODO
	int h0, w0, Comp, Pitch;
	int i, j, W, H, k;

	w0 = img.width();
	h0 = img.height();
	Comp = img.dim();
	Pitch = img.pitch();
	printf("%d,%d,%d\n", Pitch, w0, h0);
	/*for (i = 0; i < h0; i++)
	{
		for (j = 0; j < w0; j++)
		{
			printf("%d  ", img.data()[(i * w0 + j) * Comp + 3]);
		}
		printf("\n");
	}*/

	std::vector<BYTE>data(Pitch * h0);
	//data = img.bits(1);//创建一个新的data，img.data赋给data
	for (i = 0; i < h0; i++)
	{
		//std::copy_n(img.data() + i * w0 * Comp, w0 * Comp, data.data() + i * Pitch);
		for (j = 0; j < w0; j++)
		{
			for (k = 0; k < Comp; k++)
			{
				data[(i * w0 + j) * Comp + k] = img.data()[(i * w0 + j) * Comp + k];
			}
		}
	}

	for (W = w0; W > w; W--)
	{
		vector<int>Trace(h0);
		Trace = calculateEnergy_rows(data, h0, W, Comp);
		data = delete_cols(data, h0, W, Comp, Trace);
	}
	MyImage img0(data.data(), w, h0, w * Comp, Comp);//对img0进行操作，最后我们也返回img0
	//printf("%d\n", Comp);
	/*for (i = 0; i < h0; i++)
	{
		for (j = 0; j < w0; j++)
		{
			printf("%d  ", img.data()[(i * w0 + j) * Comp + 3]);
		}
		printf("\n");
	}*/
	return img0.rescale(w, h);//img.rescale(w, h);
}

void createTweakbar()
{
	//Create a tweak bar
	TwBar* bar = TwNewBar("Image Viewer");
	TwDefine(" 'Image Viewer' size='220 150' color='0 128 255' text=dark alpha=128 position='5 5'"); // change default tweak bar size and color

	TwAddVarRW(bar, "Scale", TW_TYPE_FLOAT, &M.mMeshScale, " min=0 step=0.1");

	TwAddVarRW(bar, "Image filename", TW_TYPE_STDSTRING, &imagefile, " ");
	TwAddButton(bar, "Read Image", [](void*) { readImage(imagefile); }, nullptr, "");

	TwAddVarRW(bar, "Resize Width", TW_TYPE_INT32, &resize_width, "group='Seam Carving' min=1 ");
	TwAddVarRW(bar, "Resize Height", TW_TYPE_INT32, &resize_height, "group='Seam Carving' min=1 ");
	TwAddButton(bar, "Run Seam Carving", [](void* img) {
		MyImage newimg = seamCarving(*(const MyImage*)img, resize_width, resize_height);
		uploadImage(newimg);
	},
		&img, "");
}

int main(int argc, char* argv[])
{
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { 100, 5000 });

	if (initGL(argc, argv)) {
		fprintf(stderr, "!Failed to initialize OpenGL!Exit...");
		exit(-1);
	}

	MyMesh::buildShaders();

	float x[] = { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0 };
	float uv[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
	int t[] = { 0, 1, 2, 2, 3, 0 };

	M.upload(x, 4, t, 2, uv);

	//////////////////////////////////////////////////////////////////////////
	TwInit(TW_OPENGL_CORE, NULL);
	//Send 'glutGetModifers' function pointer to AntTweakBar;
	//required because the GLUT key event functions do not report key modifiers states.
	TwGLUTModifiersFunc(glutGetModifiers);
	glutSpecialFunc([](int key, int x, int y) { TwEventSpecialGLUT(key, x, y); glutPostRedisplay(); }); // important for special keys like UP/DOWN/LEFT/RIGHT ...
	TwCopyStdStringToClientFunc([](std::string& dst, const std::string& src) {dst = src; });

	createTweakbar();

	//////////////////////////////////////////////////////////////////////////
	atexit([] { TwDeleteAllBars();  TwTerminate(); });  // Called after glutMainLoop ends

	glutTimerFunc(1, [](int) { readImage(imagefile); }, 0);

	//////////////////////////////////////////////////////////////////////////
	glutMainLoop();

	return 0;
}