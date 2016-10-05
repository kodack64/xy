
//#define USE_OPENCV

#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <string>
#include <complex>
#include <vector>
#include <random>

#ifdef USE_OPENCV
#pragma warning(disable: 4819)
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif //USE_OPENCV

#include <GL/freeglut.h>

#pragma comment(linker,"/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

using namespace std;

int L = 30;
vector<double> spin = vector<double>(L*L, 0);
vector<double> chi = vector<double>(L*L, 0);
mt19937 mt;
uniform_real_distribution<double> dist;
int step = 300;
int N = L*L;
double temp = 0.01;

const double pi = acos(0.0) * 2;

/* initialize */
bool camcont = false;
double currentTime = 0;
int lastMx = 0;
int lastMy = 0;
bool firstMouse = true;
string command = "";
bool stop = false;
double t0 = -10000;
double delta = 1e-10;

// window
int width = 800;
int height = 600; 
int fps = 60;
string windowTitle = "XY";

// camera position
double camtheta = 0; // rad
double camphi = 0; // rad
double camdist = 300000; // (nm) 0,0,0Ç©ÇÁÇÃÉJÉÅÉâÇÃãóó£
double camSpeed = 0.01; // (rad/pix)
double camscaling = 1.05; // ägëÂèkè¨éûÇÃÉXÉPÅ[Éã
double camFar = 1000000; // (nm)
double camNear = 100; // (nm)
double camFov = 5.0; // (degree)
double camsmooththeta = camtheta;
double camsmoothphi = camphi;
double camsmoothdist = camdist;
double camMinSpeed = pi / 1e4;
double camratio = 0.05; // ÉJÉÅÉâà⁄ìÆéûÇÃí«è] 1Ç≈àÍèuÇ≈à⁄ìÆ

// anime speed
double animeSpeed = 3e-6; // (ps per realSec) 1e-12

#ifdef USE_OPENCV
bool recordFlag = false;
const unsigned int cc = 3;
void* dataBuffer = NULL;
IplImage *frame;
CvVideoWriter* vw;
int recordCount = 0;
double recordTime = 1000.0; // ò^âÊéûä‘
double videoFps = 29.7;  // ìÆâÊÇÃfps
#endif //USE_OPENCV


// arrows
double arrowInterval = 800; //(nm) ÉxÉNÉgÉãï\é¶ÇÃä‘äu
int arrowCount = L; // num ï\é¶êî
double arrowHeight = arrowInterval/2.5; // (nm) ñÓÇÃâ~íåÇÃçÇÇ≥
double arrowRadius = arrowInterval/10.0; // (nm) ñÓÇÃâ~íåÇÃîºåa
double coneHeight = arrowRadius * 2; // (nm) ñÓÇÃêÊí[ÇÃçÇÇ≥
double coneRadius = arrowRadius * 3; // (nm) ñÓÇÃêÊí[ÇÃîºåa

void drawArrow(double,double);
void drawChirality();
void drawString(string);

void init() {
//	for (int i = 0; i < L*L; i++) spin[i] = dist(mt);
#ifdef USE_OPENCV
	dataBuffer = (GLubyte*)malloc(width*height*cc);
	vw = cvCreateVideoWriter("cap.avi", -1, 30.0 - 0.03, cvSize(width, height));
	recordFlag = true;
	recordCount = 0;
	recordTime = 120.0;
#endif
}

double ene(int i, double s) {
	int x = i%L;
	int y = i / L;
	int dx[4] = { 0,1,0,-1 };
	int dy[4] = { 1,0,-1,0 };
	double sum = 0;
	for (int i = 0; i < 4; i++) {
		int nx = (x + dx[i] + L) % L;
		int ny = (y + dy[i] + L) % L;
		sum += -cos(2.0 * pi*(spin[nx + ny*L] - s));
	}
	return sum;
}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	// locate camera
	double cphi = min(max(camsmoothphi, delta), pi - delta);
	double ctheta = camsmooththeta;
	double eyex = camsmoothdist*cos(ctheta)*sin(cphi);
	double eyez = camsmoothdist*sin(ctheta)*sin(cphi);
	double eyey = camsmoothdist*cos(cphi);
	gluLookAt(eyex, eyey, eyez,0, 0, 0,0, 1, 0);

	// move camera
	if (fabs(camsmooththeta - camtheta) < camMinSpeed) {
		camsmooththeta = camtheta;
	} else {
		camsmooththeta = camratio*camtheta + (1-camratio)*camsmooththeta;
	}
	if (fabs(camsmoothphi - camphi) < camMinSpeed) {
		camsmoothphi = camphi;
	} else {
		camsmoothphi = camratio*camphi + (1 - camratio)*camsmoothphi;
	}
	if (fabs(camsmoothdist - camdist) < camMinSpeed) {
		camsmoothdist = camdist;
	} else {
		camsmoothdist = camratio*camdist + (1 - camratio)*camsmoothdist;
	}
	double px, py;

	// ambient lighting
	GLfloat pos0[] = { 0, 1000, 0, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, pos0);
	GLfloat pos1[] = { 0, -1000, 0, 0.0 };
	glLightfv(GL_LIGHT1, GL_POSITION, pos1);
	GLfloat pos2[] = { 1000, 0, 0, 0.0 };
	glLightfv(GL_LIGHT2, GL_POSITION, pos2);

	for (int i = 0; i < arrowCount; i++) {
		for (int j = 0; j < arrowCount; j++) {
			glPushMatrix();
			{
				px = cos(2*pi*spin[j+arrowCount*i]);
				py = sin(2*pi*spin[j+arrowCount*i]);
				double length = 1.0;
				double rot = atan2(py, px);
				glTranslated((j - arrowCount / 2)*arrowInterval, 0, (i - arrowCount / 2)*arrowInterval);
				glRotated(rot / 2 / pi * 360, 0, 1, 0);
				drawArrow(length,spin[j+arrowCount*i]);
			}
			glPopMatrix();
		}
	}
	drawChirality();
	drawString("T="+to_string(temp));

	glFlush();
	glutSwapBuffers();
#ifdef USE_OPENCV
	// add frame to video when record
	if (recordFlag) {
		frame = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, cc);
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, dataBuffer);
		GLubyte* p = static_cast<GLubyte*>(dataBuffer);
		for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				frame->imageData[(height - j - 1) * frame->widthStep + i * 3 + 0] = *p;
				frame->imageData[(height - j - 1) * frame->widthStep + i * 3 + 1] = *(p + 1);
				frame->imageData[(height - j - 1) * frame->widthStep + i * 3 + 2] = *(p + 2);
				p += cc;
			}
		}
		cvWriteFrame(vw, frame);
		cvReleaseImage(&frame);
		recordCount++;

		if (1.0*recordCount / fps > recordTime) {
			cvReleaseVideoWriter(&vw);
			vw = NULL;
			free(dataBuffer);
			recordFlag = false;
		}
	}
#endif //USE_OPENCV
}

int getId(int x,int y) {
	return ((y+L)%L)*L + (x+L)%L;
}
void timer(int value) {
	glutTimerFunc(1000/fps, timer, 0);
	if (!stop) {
		currentTime += 1000.0 / fps * animeSpeed;
		for (int i = 0; i < step; i++) {
			int id = mt() % N;
			double ns = dist(mt);
			double dif = ene(id, ns) - ene(id, spin[id]);
			double prob = 1. / (1. + exp(dif / temp));
			if (prob > dist(mt)) spin[id] = ns;
		}
		for (int y = 0; y < L; y++) {
			for (int x = 0; x < L; x++) {
				int d[2][5] = {
					{0,0,1,1,0},
					{0,1,1,0,0}
				};
				double sum = 0;
				for (int ci = 0; ci < 4; ci++) {
					int id1 = getId(x + d[0][ci], y + d[1][ci]);
					int id2 = getId(x + d[0][ci+1], y + d[1][ci+1]);
					double x1 = cos(2.*pi*spin[id1]);
					double y1 = sin(2.*pi*spin[id1]);
					double x2 = cos(2.*pi*spin[id2]);
					double y2 = sin(2.*pi*spin[id2]);
					sum += x1*y2 - y1*x2;
				}
				chi[getId(x, y)] = sum/4;
			}
		}
	}
	glutPostRedisplay();
}

void keyboard(unsigned char key, int mx, int my) {
	if (key == VK_ESCAPE) {
		glutLeaveMainLoop();
	}
	if (key == VK_SPACE) {
		glutFullScreenToggle();
	}
	// camera rotation
	if (key == 'W') {
		camphi -= pi/32;
		if (camphi < 0)camphi = 0;
	}
	if (key == 'S') {
		camphi += pi/32;
		if (camphi > pi)camphi = pi;
	}
	if (key == 'D') {
		camtheta += pi/16;
	}
	if (key == 'A') {
		camtheta -= pi/16;
	}

	// camera zooming
	if (key == 'F') {
		camdist *= 1.05;
	}
	if (key == 'R') {
		camdist /= 1.05;
	}

	// move camera to registered position
	if (key == 'I') {
		camtheta = 0;
		camphi = 0;
	}
	if (key == 'K') {
		camtheta = 0;
		camphi = pi/2;
	}
	if (key == 'J') {
		camtheta = pi / 2;
		camphi = pi / 2;
	}
	if (key == 'L') {
		camtheta = -pi/2;
		camphi = pi/2;
	}
	if (key == 'U') {
		camtheta = pi *3/ 16;
		camphi = pi *6/ 16;
	}
	if (key == 'O') {
		camtheta = -pi *3/ 16;
		camphi = pi *6/ 16;
	}
	if (key == '+') {
		temp *= 1.1;
	}
	if (key == '-') {
		temp /= 1.1;
	}
}



void reshape(int w, int h) {
	width = w;
	height = h;
	// reset camera view
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(camFov, 1.0*width / height, camNear, camFar);
	glMatrixMode(GL_MODELVIEW);
}

// camera scaling
void mouseWheel(int wheelNumber,int direction,int x,int y) {
	if (direction == 1) {
		camdist *= camscaling;
	} else {
		camdist /= camscaling;
	}
}
// camera lock
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_UP) {
			camcont = false;
		} else if(state==GLUT_DOWN){			
			camcont = true;
			lastMx = x;
			lastMy = y;
		}
	}
}
// camera move
void motion(int x, int y) {
	if (camcont) {
		int difx = x - lastMx;
		int dify = y - lastMy;
		camtheta += difx*camSpeed;
		camphi -= dify*camSpeed;
		lastMx = x;
		lastMy = y;
		if (camphi < 0) camphi = 0;
		if (camphi > pi) camphi = pi;
	}
}

int main(int argc,char** argv) {
	glutInit(&argc, argv);;
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow(windowTitle.c_str());

	// callback
	glutDisplayFunc(display);
	glutTimerFunc(1000/fps,timer,0);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseWheelFunc(mouseWheel);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	// alpha setting
	glClearColor(0, 0, 0, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glShadeModel(GL_SMOOTH);

	// lighting
	glEnable(GL_LIGHTING);
	{
		glEnable(GL_LIGHT0);
		GLfloat lightpos[] = { 0.0f, 1000.0f,0.0f, 0.0f };
		GLfloat lightdif[] = { 0.6f, 0.6f, 0.6f, 1.0f };
		GLfloat lightamb[] = { 0.4f, 0.4f, 0.4f, 1.0f };
		GLfloat lightspe[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightspe);
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
	}
	init();
	glutMainLoop();
#ifdef USE_OPENCV
	if (vw != NULL) {
		cvReleaseVideoWriter(&vw);
	}
#endif //USE_OPENCV
}

void drawString(string sstr) {
	const char* str = sstr.c_str();
	glPushMatrix();
	{
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		{
			glDisable(GL_LIGHTING);
			glLoadIdentity();
			gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));
			glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glColor4d(1, 1, 1, 1);
			glRasterPos2d(20, 20);
			while (*str) {
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *str);
				str++;
			}
			glPopAttrib();
			glEnable(GL_LIGHTING);
		}
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

void drawArrow(double amplitude,double s) {
/*	GLfloat dif[] = { 0.2f, 0.8f, 0.2f, 1.0f };*/
	float r, g, b;
	r = float(fabs(s-0.5));
	g = float(min(fabs(s-0.8),fabs(s+0.2)));
	b = float(min(fabs(s-0.2),fabs(s+0.8)));
	GLfloat dif[] = { r, g, b, 1.0f };
	GLfloat amb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat spe[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat emi[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLfloat shi = 50;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shi);

	glRotated(90.0, 0, 0, 1);
	glPushMatrix();
	{
		glRotated(90.0, 1, 0, 0);
		glutSolidCylinder(arrowRadius, arrowHeight * amplitude, 20, 20);
	}
	glPopMatrix();

	glPushMatrix();
	{
		glTranslated(0, -arrowHeight * amplitude, 0);
		glRotated(90.0, 1, 0, 0);
		glutSolidCone(coneRadius, coneHeight, 20, 20);
	}
	glPopMatrix();
}

void drawChirality() {
	glEnable(GL_BLEND);
	glPushMatrix();
	{
		glPushMatrix();
		{
			GLfloat spe[] = { 0.5f, 0.5f, 0.5f, 0.9f };
			GLfloat emi[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			GLfloat amb[] = { 0.6f, 0.6f, 0.6f, 0.9f };
			GLfloat shi = 50;
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shi);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);

			glRotated(90.0, 1, 0, 0);
			glTranslated(arrowInterval, arrowInterval, 3000.0);
			double os = arrowInterval;
			glBegin(GL_QUADS);
			for (int x = 0; x<L; x++) {
				for (int y = 0;y<L;y++) {
					int id = x + y*L;
					float r = pow(max(0.f, float(chi[id])),0.5f);
					float g = pow(max(0.f, -float(chi[id])),0.5f);
					GLfloat dif[] = { r, g, 0.0f, max(r,g) };
					glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);

					int i = x - L / 2;
					int j = y - L / 2;
					glNormal3d(0, 0, -1);
					glVertex3d(os*i, os*j, 0);
					glVertex3d(os*i, os*(j - 1), 0);
					glVertex3d(os*(i - 1), os*(j - 1), 0);
					glVertex3d(os*(i - 1), os*j, 0);
				}
			}
			glEnd();
		}
		glPopMatrix();
	}
	glPopMatrix();
	glDisable(GL_BLEND);
}
