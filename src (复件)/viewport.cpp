#include "viewport.h"

#include "CMU462.h"
#include <iostream>

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 4 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.
  this->x = x;
  this->y = y;
  this->span = span; 
	setNormEntry(0,0,0.5/span);
	setNormEntry(0,1,0.0);
	setNormEntry(0,2,0.5-0.5*x/span);
	setNormEntry(1,0,0.0);
	setNormEntry(1,1,0.5/span);
	setNormEntry(1,2,0.5-0.5*y/span);
	setNormEntry(2,0,0.0);
	setNormEntry(2,1,0.0);
	setNormEntry(2,2,1.0);
	//set_canvas_to_norm(mTemp);
	//std::cout << "viewbox set to: x=" << x << ",y=" << y << ",span=" << span << std::endl;
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  //std::cout << "viewbox updated to: x=" << x << ",y=" << y << ",span=" << span << std::endl;
  set_viewbox( x, y, span );
}

} // namespace CMU462
