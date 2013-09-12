/*
Copyright (c) 2010-2013, Paul Houx - All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

This file is part of Cinder-Warping.

Cinder-Warping is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Cinder-Warping is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cinder-Warping.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "WarpingApp.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;

// add image
void WarpingApp::addImage()
{
	try 
	{
		fs::path p = getOpenFilePath( "", ImageIo::getLoadExtensions() );
		// an empty string means the user canceled
		if( ! p.empty() ) 
		{ 
			mChannel = Channel32f( loadImage( p ) );
			mImage = mChannel;
		}
	}
	catch( ... ) {
		console() << "Unable to load the image." << std::endl;
	}
}
void WarpingApp::addMovie()
{
	console() << "Add Movie" << std::endl;
	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() )
		loadMovieFile( moviePath );
}

void WarpingApp::fileDrop( FileDropEvent event )
{
	loadMovieFile( event.getFile( 0 ) );
}
void WarpingApp::loadMovieFile( const fs::path &moviePath )
{
	qtime::MovieGl movie;

	try 
	{
		movie = qtime::MovieGl( moviePath );
		movie.setVolume(0.0f);
		console() << "Dimensions:" << movie.getWidth() << " x " << movie.getHeight() << std::endl;
		console() << "Duration:  " << movie.getDuration() << " seconds" << std::endl;
		console() << "Frames:    " << movie.getNumFrames() << std::endl;
		console() << "Framerate: " << movie.getFramerate() << std::endl;
		movie.setLoop( true, false );

		mMovies.push_back( movie );
		movie.play();
		mLastPath = moviePath;
	}
	catch( ... )
	{
		console() << "Unable to load the movie." << std::endl;
		return;
	}

}
void WarpingApp::prepareSettings( Settings *settings )
{

	settings->setWindowSize( 450, 400 );
	settings->setFrameRate( 60.0f );
	settings->enableConsoleWindow();
}
void WarpingApp::setup()
{
	mUseBeginEnd = true;
	updateWindowTitle();

	// initialize warps
	fs::path settings = getAssetPath("") / "warps.xml";
	if( fs::exists( settings ) )
	{
		// load warp settings from file if one exists
		mWarps = Warp::readSettings( loadFile( settings ) );
	}
	else
	{
		// otherwise create a warp from scratch
		mWarps.push_back( WarpPerspectiveBilinearRef( new WarpPerspectiveBilinear() ) );
	}

	// load test image
	try
	{ 
		mImage = gl::Texture( loadImage( loadAsset("voletshaut.jpg") ) );

		mSrcArea = mImage.getBounds();

		// adjust the content size of the warps
		Warp::setSize( mWarps, mImage.getSize() );
	}
	catch( const std::exception &e )
	{
		console() << e.what() << std::endl;
	}
	// Display sizes
	mMainDisplayWidth = Display::getMainDisplay()->getWidth();
	mRenderX = mMainDisplayWidth;
	mRenderY = 0;
	for (auto display : Display::getDisplays() )
	{
		//std::cout << "Reso:" << display->getHeight() << "\n"; 
		mRenderWidth = display->getWidth();
		mRenderHeight = display->getHeight();
	}
	mParams = params::InterfaceGl( "Params", Vec2i( 400, 300 ) );
	mParams.addParam( "Full screen",			&mFullScreen,									"key=f"		);
	mParams.addParam( "Render Window X",		&mRenderX,										"" );
	mParams.addParam( "Render Window Y",		&mRenderY,										"" );
	mParams.addParam( "Render Window Width",	&mRenderWidth,									"" );
	mParams.addParam( "Render Window Height",	&mRenderHeight,									"" );
	mParams.addButton( "Create window",			bind( &WarpingApp::createNewWindow, this ),	"key=n" );
	mParams.addButton( "Delete windows",		bind( &WarpingApp::deleteWindows, this ),		"key=d" );
	mParams.addButton( "Add movie",				bind( &WarpingApp::addMovie, this ),			"key=o" );
	mParams.addButton( "Open image",			bind( &WarpingApp::addImage, this ),			"key=i" );
	mParams.addButton( "Quit",					bind( &WarpingApp::shutdown, this ),			"key=q" );

	//store window
	controlWindow = this->getWindow();
	int uniqueId = getNumWindows();
	controlWindow->getSignalClose().connect(
		[uniqueId,this] { shutdown(); this->console() << "You quit console window #" << uniqueId << std::endl; }
	);
}
void WarpingApp::createNewWindow()
{
	WindowRef renderWindow = createWindow( Window::Format().size( mRenderWidth, mRenderHeight ) );
	// create instance of the window and store in vector
	RenderWindow rWin = RenderWindow("name", mRenderWidth, mRenderHeight, renderWindow);	
	renderWindows.push_back( rWin );
	renderWindow->setPos(mRenderX, mRenderY);
	renderWindow->setBorderless();
	renderWindow->setAlwaysOnTop();

	HWND hWnd = (HWND)renderWindow->getNative();

	HRESULT hr = S_OK;
	// Create and populate the Blur Behind structure
	DWM_BLURBEHIND bb = {0};

	// Enable Blur Behind and apply to the entire client area
	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = true;
	bb.hRgnBlur = NULL;

	// Apply Blur Behind
	hr = DwmEnableBlurBehindWindow(hWnd, &bb);
	if (SUCCEEDED(hr))
	{
		HRESULT hr = S_OK;

		// Set the margins, extending the bottom margin.
		MARGINS margins = {-1};

		// Extend the frame on the bottom of the client area.
		hr = DwmExtendFrameIntoClientArea(hWnd,&margins);
	}

	// for demonstration purposes, we'll connect a lambda unique to this window which fires on close
	int uniqueId = getNumWindows();
	renderWindow->getSignalClose().connect(
		[uniqueId,this] { shutdown(); this->console() << "You closed window #" << uniqueId << std::endl; }
	);

}
void WarpingApp::deleteWindows()
{
	for ( auto wRef : renderWindows ) DestroyWindow( (HWND)wRef.mWRef->getNative() );
}
void WarpingApp::shutdown()
{
	// save warp settings
	fs::path settings = getAssetPath("") / "warps.xml";
	Warp::writeSettings( mWarps, writeFile( settings ) );
	// needed?
	WarpingApp::quit();
}

void WarpingApp::update()
{
	// there is nothing to update
}

void WarpingApp::draw()
{
	// clear the window and set the drawing color to white
	// clear out the window with transparency
	gl::clear( ColorAf( 0.0f, 0.0f, 0.0f, 0.0f ) );

	// Draw on render window only
	if (app::getWindow() == controlWindow)	
	{
		// Draw the params on control window only
		mParams.draw();
	}
	else
	{
		gl::color( Color::white() );
		if( mImage ) 
		{
			// iterate over the warps and draw their content
			for(WarpConstIter itr=mWarps.begin();itr!=mWarps.end();++itr)
			{
				// create a readable reference to our warp, to prevent code like this: (*itr)->begin();
				WarpRef warp( *itr );

				// there are two ways you can use the warps:
				if( mUseBeginEnd )
				{
					// a) issue your draw commands between begin() and end() statements
					warp->begin();

					// in this demo, we want to draw a specific area of our image,
					// but if you want to draw the whole image, you can simply use: gl::draw( mImage );
					gl::draw( mImage, mSrcArea, warp->getBounds() );

					warp->end();
				}
				else
				{
					// b) simply draw a texture on them (ideal for video)
					//QTime
					int totalWidth = 0;
					for( size_t m = 0; m < mMovies.size(); ++m )
						totalWidth += mMovies[m].getWidth();

					int drawOffsetX = 0;
					for( size_t m = 0; m < mMovies.size(); ++m ) {
						float relativeWidth = mMovies[m].getWidth() / (float)totalWidth;
						gl::Texture texture = mMovies[m].getTexture();
						if( texture ) {
							float drawWidth = getWindowWidth() * relativeWidth;
							float drawHeight = ( getWindowWidth() * relativeWidth ) / mMovies[m].getAspectRatio();
							float x = drawOffsetX;
							float y = ( getWindowHeight() - drawHeight ) / 2.0f;			

							gl::color( Color::white() );
							//gl::draw( texture, Rectf( x, y, x + drawWidth, y + drawHeight ) );
							warp->draw( texture, mSrcArea );
							texture.disable();
							//drawFFT( mMovies[m], x, y, drawWidth, drawHeight );
						}
						drawOffsetX += getWindowWidth() * relativeWidth;
					}
					// in this demo, we want to draw a specific area of our image,
					// but if you want to draw the whole image, you can simply use: warp->draw( mImage );
					//warp->draw( mImage, mSrcArea );
				}
			}
		}
	}
}

void WarpingApp::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize( mWarps );
}

void WarpingApp::mouseMove( MouseEvent event )
{
	// pass this mouse event to the warp editor first
	if( ! Warp::handleMouseMove( mWarps, event ) )
	{
		// let your application perform its mouseMove handling here
	}
}

void WarpingApp::mouseDown( MouseEvent event )
{
	// pass this mouse event to the warp editor first
	if( ! Warp::handleMouseDown( mWarps, event ) )
	{
		// let your application perform its mouseDown handling here
	}
}

void WarpingApp::mouseDrag( MouseEvent event )
{
	// pass this mouse event to the warp editor first
	if( ! Warp::handleMouseDrag( mWarps, event ) )
	{
		// let your application perform its mouseDrag handling here
	}
}

void WarpingApp::mouseUp( MouseEvent event )
{
	// pass this mouse event to the warp editor first
	if( ! Warp::handleMouseUp( mWarps, event ) )
	{
		// let your application perform its mouseUp handling here
	}
}

void WarpingApp::keyDown( KeyEvent event )
{
	// pass this key event to the warp editor first
	if( ! Warp::handleKeyDown( mWarps, event ) )
	{
		// warp editor did not handle the key, so handle it here
		switch( event.getCode() )
		{
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen( ! isFullScreen() );
			break;
		case KeyEvent::KEY_w:
			// toggle warp edit mode
			Warp::enableEditMode( ! Warp::isEditModeEnabled() );
			break;
		case KeyEvent::KEY_a:
			// toggle drawing a random region of the image
			if( mSrcArea.getWidth() != mImage.getWidth() || mSrcArea.getHeight() != mImage.getHeight() )
				mSrcArea = mImage.getBounds();
			else
			{
				int x1 = Rand::randInt( 0, mImage.getWidth() - 150 );
				int y1 = Rand::randInt( 0, mImage.getHeight() - 150 );
				int x2 = Rand::randInt( x1 + 150, mImage.getWidth() );
				int y2 = Rand::randInt( y1 + 150, mImage.getHeight() );
				mSrcArea = Area( x1, y1, x2, y2 );
			}
			break;
		case KeyEvent::KEY_SPACE:
			// toggle drawing mode
			mUseBeginEnd = !mUseBeginEnd;
			updateWindowTitle();
			break;
		}
	}
}

void WarpingApp::keyUp( KeyEvent event )
{
	// pass this key event to the warp editor first
	if( ! Warp::handleKeyUp( mWarps, event ) )
	{
		// let your application perform its keyUp handling here
	}
}

void WarpingApp::updateWindowTitle()
{
	if(mUseBeginEnd)
		getWindow()->setTitle("Warping Sample - Using begin() and end()");
	else
		getWindow()->setTitle("Warping Sample - Using draw()");
}

CINDER_APP_BASIC( WarpingApp, RendererGl )
