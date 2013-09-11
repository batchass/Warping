#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/params/Params.h"
#include "dwmapi.h"
#include "WarpBilinear.h"
#include "WarpPerspective.h"
#include "WarpPerspectiveBilinear.h"
#include "OscListener.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;

// RenderWindow class
class RenderWindow
{
	public:
		RenderWindow( string name, int width, int height, WindowRef wRef )
			: mName( name ), mWidth ( width ), mHeight ( height ), mWRef ( wRef )
		{
			
		}
		
		WindowRef mWRef;
		
	private:
		string mName;
		int mWidth;
		int mHeight;
		
};

class WarpingApp : public AppBasic {
public:	
	void prepareSettings( AppBasic::Settings *settings );
	void setup();
	void shutdown();
	void update();
	void draw();
	
	void resize();
	
	void mouseMove( MouseEvent event );	
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void mouseUp( MouseEvent event );	
	
	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );

	void updateWindowTitle();
	// image
	void addImage();
	// qtime
	void addMovie();
	void addActiveMovie( qtime::MovieGl movie );
	void loadMovieFile( const fs::path &path );
	// windows mgmt
	void createNewWindow();
	void deleteWindows();
	vector<RenderWindow>	renderWindows;
	WindowRef			controlWindow;

private:
	gl::Texture	mImage;
	WarpList	mWarps;

	bool		mUseBeginEnd;

	Area		mSrcArea;
	// all of the actively playing movies
	vector<qtime::MovieGl>	mMovies;
	fs::path				mLastPath;
	// windows and params
	int						mMainDisplayWidth;
	int						mRenderX;
	int						mRenderY;
	int						mRenderWidth;
	int						mRenderHeight;
	float					mFrameRate;
	bool					mFullScreen;
	bool					mFluid;
	ci::params::InterfaceGl	mParams;
	//Particles
	Channel32f mChannel;

};