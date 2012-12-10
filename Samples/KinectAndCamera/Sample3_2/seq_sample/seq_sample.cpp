/*---------------------------------------------------------------------------
                  �A��MQO�t�@�C���ɂ��A�j���[�V�����\��
---------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <AR/ar.h>
#include <AR/param.h>
#include <AR/video.h>
#include <AR/gsub.h>
#include "GLMetaseq.h"

#include "MyKinect.h"
#include "HandDetectorNite.h"

// �O���[�o���ϐ�
char	*vconf_name		= "Data/WDM_camera_flipV.xml";	// �r�f�I�f�o�C�X�̐ݒ�t�@�C��
char	*cparam_name    = "Data/MyCameraParameter4.dat";		// �J�����p�����[�^�t�@�C��
char	*patt_name      = "Data/patt.ninja";			// �p�^�[���t�@�C��
char	*seq_name		= "Sequence/jump_%d.mqo";		// MQO�t�@�C��

int		patt_id;						// �p�^�[����ID
double	patt_trans[3][4];				// ���W�ϊ��s��
double	patt_center[2]	= { 0.0, 0.0 };	// �p�^�[���̒��S���W
double	patt_width		= 80.0;			// �p�^�[���̃T�C�Y�i�P�ʁFmm�j
int		thresh			= 100;			// 2�l����臒l

MQO_SEQUENCE	mqo_seq;		// �V�[�P���X
int				n_frame = 70;	// �t���[����

int g_Move = 0;

bool				drawFromKinect = false;

MyKinect			g_MyKinect;
HandDetectorNite	g_HandDetectorNite;

// �v���g�^�C�v�錾
void MainLoop(void);
void DrawObject(void);
void MouseEvent(int button, int state, int x, int y);
void KeyEvent(unsigned char key, int x, int y);
void Cleanup(void);
void mySetLight(void);

void LeftMove()
{
	g_Move = 1;
}

void RightMove()
{
	g_Move = 2;
}

//===========================================================================
// main�֐�
//===========================================================================
int main( int argc, char **argv )
{
	ARParam	cparam;			// �J�����p�����[�^
	ARParam	wparam;			// �J�����p�����[�^�i��Ɨp�ϐ��j
	int		xsize = 640, ysize = 480;	// �摜�T�C�Y

	// GLUT�̏�����
	glutInit( &argc, argv );
	
	g_MyKinect.Init();
	g_HandDetectorNite.Init(&g_MyKinect.context);
	g_HandDetectorNite.LeftSwipePointerFunc = &LeftMove;
	g_HandDetectorNite.RightSwipePointerFunc = &RightMove;


	// �r�f�I�f�o�C�X�̐ݒ�
	if ( arVideoOpen( vconf_name ) < 0 ) {
		printf("�r�f�I�f�o�C�X�̃G���[");
		return -1;
	}

	// �J�����p�����[�^�̐ݒ�
	if ( arVideoInqSize( &xsize, &ysize ) < 0 ) {
		printf("�摜�T�C�Y���擾�ł��܂���ł���\n");
		return -1;
	}
	if ( arParamLoad( cparam_name, 1, &wparam ) < 0 ) {
		printf("�J�����p�����[�^�̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}
	arParamChangeSize( &wparam, xsize, ysize, &cparam );
	arInitCparam( &cparam );

	// �p�^�[���t�@�C���̃��[�h
	if ( (patt_id = arLoadPatt(patt_name)) < 0 ) {
		printf("�p�^�[���t�@�C���̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}

	// �E�B���h�E�̐ݒ�
	argInit( &cparam, 1.0, 0, 0, 0, 0 );

	// GLMetaseq�̏�����
	mqoInit();

	// �V�[�P���X�̓ǂݍ���
	printf("�V�[�P���X�̓ǂݍ��ݒ�...");
	mqo_seq = mqoCreateSequence( seq_name, n_frame, 1.0 );
	if ( mqo_seq.n_frame <= 0 ) {
		printf("�V�[�P���X�̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}
	printf("����\n");

	// �r�f�I�L���v�`���̊J�n
	arVideoCapStart();
	
	g_MyKinect.StartGeneratingAll();
	// ���C�����[�v�̊J�n
	argMainLoop( MouseEvent, KeyEvent, MainLoop );

	return 0;
}


//===========================================================================
// ���C�����[�v�֐�
//===========================================================================
void MainLoop(void)
{
	ARUint8			*image;
	ARMarkerInfo	*marker_info;
	int				marker_num;
	int				j, k;

	g_MyKinect.Update();
	g_HandDetectorNite.Update(&g_MyKinect.context);

	// �J�����摜�̎擾
	if(drawFromKinect)
	{
		if ( (image = (ARUint8 *)g_MyKinect.GetBGRA32Image()) == NULL ) {
			arUtilSleep( 2 );
			return;
		}
	}
	else
	{
		if ( (image = arVideoGetImage()) == NULL ) {
			arUtilSleep( 2 );
			return;
		}
	}

	// �J�����摜�̕`��
	argDrawMode2D();
	argDispImage( image, 0, 0 );

	// �}�[�J�̌��o�ƔF��
	if ( arDetectMarker( image, thresh, &marker_info, &marker_num ) < 0 ) {
		Cleanup();
		exit(0);
	}

	// ���̉摜�̃L���v�`���w��
	arVideoCapNext();

	// �}�[�J�̐M���x�̔�r
	k = -1;
	for( j = 0; j < marker_num; j++ ) {
		if ( patt_id == marker_info[j].id ) {
			if ( k == -1 ) k = j;
			else if ( marker_info[k].cf < marker_info[j].cf ) k = j;
		}
	}

	if ( k != -1 ) {
		// �}�[�J�̈ʒu�E�p���i���W�ϊ��s��j�̌v�Z
		arGetTransMat( &marker_info[k], patt_center, patt_width, patt_trans );

		// 3D�I�u�W�F�N�g�̕`��
		DrawObject();
	}

	// �o�b�t�@�̓��e����ʂɕ\��
	argSwapBuffers();
}


//===========================================================================
// �����̐ݒ���s���֐�
//===========================================================================
void mySetLight(void)
{
	GLfloat light_diffuse[]  = { 0.9, 0.9, 0.9, 1.0 };	// �g�U���ˌ�
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };	// ���ʔ��ˌ�
	GLfloat light_ambient[]  = { 0.3, 0.3, 0.3, 0.1 };	// ����
	GLfloat light_position[] = { 100.0, -200.0, 200.0, 0.0 };	// �ʒu�Ǝ��

	// �����̐ݒ�
	glLightfv( GL_LIGHT0, GL_DIFFUSE,  light_diffuse );	 // �g�U���ˌ��̐ݒ�
	glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular ); // ���ʔ��ˌ��̐ݒ�
	glLightfv( GL_LIGHT0, GL_AMBIENT,  light_ambient );	 // �����̐ݒ�
	glLightfv( GL_LIGHT0, GL_POSITION, light_position ); // �ʒu�Ǝ�ނ̐ݒ�

//	glShadeModel( GL_SMOOTH );	// �V�F�[�f�B���O�̎�ނ̐ݒ�
	glEnable( GL_LIGHT0 );		// �����̗L����
}


//===========================================================================
// 3D�I�u�W�F�N�g�̕`����s���֐�
//===========================================================================
void DrawObject(void)
{
	static int k = 0;	// �`�悷��t���[���̔ԍ�
	double	gl_para[16];

	// 3D�I�u�W�F�N�g��`�悷�邽�߂̏���
	argDrawMode3D();
	argDraw3dCamera( 0, 0 );

	// ���W�ϊ��s��̓K�p
	argConvGlpara( patt_trans, gl_para );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixd( gl_para );

	// 3D�I�u�W�F�N�g�̕`��
	glClear( GL_DEPTH_BUFFER_BIT );		// Z�o�b�t�@�̏�����
	glEnable( GL_DEPTH_TEST );			// �B�ʏ����̓K�p

	mySetLight();						// �����̐ݒ�
	glEnable( GL_LIGHTING );			// �����̓K�p

	glPushMatrix();
		if(g_Move == 1)
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
		glRotatef( 90.0, 1.0, 0.0, 0.0 );	// ���f���𗧂�����
		mqoCallSequence( mqo_seq, k );		// �w��t���[���̕`��
	glPopMatrix();

	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );

	// �t���[���ԍ��̃J�E���g
	if(g_Move != 0)
		k++;
	if ( k >= n_frame )
	{
		k = 0;
		g_Move = 0;
	}
}


//===========================================================================
// �}�E�X���͏����֐�
//===========================================================================
void MouseEvent(int button, int state, int x, int y)
{
	// ���͏�Ԃ�\��
	printf("�{�^��:%d ���:%d ���W:(x,y)=(%d,%d) \n", button, state, x, y);
}


//===========================================================================
// �L�[�{�[�h���͏����֐�
//===========================================================================
void KeyEvent( unsigned char key, int x, int y)
{
	// ESC�L�[����͂�����A�v���P�[�V�����I��
    if ( key == 0x1b ) {
        Cleanup();
        exit(0);
    }
	if(key == '1')
		g_Move =1;
	if(key == '2')
		g_Move = 2;
	if(key == 'k')
		drawFromKinect = true;
	if(key == 'c')
		drawFromKinect = false;
}


//===========================================================================
// �I�������֐�
//===========================================================================
void Cleanup(void)
{
    arVideoCapStop();	// �r�f�I�L���v�`���̒�~
    arVideoClose();		// �r�f�I�f�o�C�X�̏I��
    argCleanup();		// �O���t�B�b�N�����̏I��
	
	g_MyKinect.Exit();
	g_HandDetectorNite.Exit();

	mqoDeleteSequence( mqo_seq );	// �V�[�P���X�̍폜
	mqoCleanup();					// GLMetaseq�̏I������
}