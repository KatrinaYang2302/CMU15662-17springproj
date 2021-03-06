#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stack>

#include "triangulation.h"

using namespace std;

namespace CMU462 {

inline double mn(double a, double b){
	return (a>b)?b:a;
}

inline double mx(double a, double b){
	return (a>b)?a:b;
}

inline double mn(double a, double b, double c){
	return mn(mn(a,b),c);
}

inline double mx(double a, double b, double c){
	return mx(mx(a,b),c);
}

int sign(double x1, double y1, double x2, double y2, double x3, double y3){
	double q = x1*y2 + x2*y3 + x3*y1 - x2*y1 - x3*y2 - x1*y3;
	if(q < 0) return -1;
	if(q > 0) return 1;
	return 0;
}

bool inside(double xTar, double yTar, double x1, double y1, double x2, double y2, double x3, double y3){
	int res1 = sign(xTar,yTar,x1,y1,x2,y2);
	int res2 = sign(xTar,yTar,x2,y2,x3,y3);
	int res3 = sign(xTar,yTar,x3,y3,x1,y1);
	return (res1>=0 && res2>=0 && res3>=0) || (res1<=0 && res2<=0 && res3<=0);
}

// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {
	//cout << Color::Black << endl;

  // set top level transformation
	while(!transStk.empty()) transStk.pop();
  	transformation = canvas_to_screen;
	transStk.push(transformation);
	//cout << "stack size:" << transStk.size() << endl;
	//cout << transformation << endl;
	if(wei_target){
		for(int i = 0; i < 4 * sample_rate * sample_rate * target_w * target_h; i++){
			wei_target[i] = 0;
			//else wei_target[i] = 255;
		}
	}
  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t s_rate ) {

  // Task 3: 
  // You may want to modify this for supersampling support
 	this->sample_rate = 1 << (s_rate - 1);
	
	if(wei_target){
		delete [] wei_target;
	}
	wei_target = new unsigned char[4 * sample_rate * sample_rate * target_w * target_h];	
	for(int i = 0; i < 4 * sample_rate * sample_rate * target_w * target_h; i++){
		wei_target[i] = 0;
	}
	wei_target_w = target_w * sample_rate;
	wei_target_h = target_h * sample_rate;
	//cout << "sample_rate changed to: " << sample_rate << endl;
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 3: 
  // You may want to modify this for supersampling support
	//cout << "Entering set_render_target" << endl;
	this->render_target = render_target;
	this->target_w = width;
	this->target_h = height;
	wei_target = new unsigned char[4 * target_w * target_h];
	for(int i = 0; i < 4 * target_w * target_h; i++){
		wei_target[i] = 0;
	}
	wei_target_w = target_w;
	wei_target_h = target_h;
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 4 (part 1):
  // Modify this to implement the transformation stack
	transStk.push(transStk.top() * element->transform);
	transformation = transStk.top();
	//cout << "in draw element, stack size: " << transStk.size() << endl;
  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }
	transStk.pop();
	transformation = transStk.top();
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor, false );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 
	Color cBound = ellipse.style.strokeColor, cIn = ellipse.style.fillColor;
	rasterize_ellipse(ellipse.center, ellipse.radius, cBound, cIn);
	
}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color, bool tag = true) {
	//cout << "point rasterizing: x = " << x << ", y = " << y << endl;
	//if tag == true, use line mode (default)
	//else use point mode
  // fill in the nearest pixel
	
	if(!tag){
		//point mode: expands all values

		//cout << "point mode" << endl;

  		int sx = (int) floor(x);
  		int sy = (int) floor(y);

  		// check bounds
  		if ( sx < 0 || sx >= target_w ) return;
  		if ( sy < 0 || sy >= target_h ) return;

  		// fill sample - NOT doing alpha blending!
  		//render_target[4 * (sx + sy * target_w) + 0] = (uint8_t) (color.r * 255);
  		//render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
  		//render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
  		//render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);
		for(int i = 0; i < sample_rate; i++){
			for(int j = 0; j < sample_rate; j++){
				int weisx = sx * sample_rate + i, weisy = sy * sample_rate + j;
				int pos = 4 * (weisx + weisy * wei_target_w);
				double buf[4];
				for(int q = 0; q < 4; q++){
					buf[q] = (double) wei_target[pos + q] / 255.0;
				}
				buf[3] = 1 - (1-color.a) * (1-buf[3]);
				buf[0] = (1-color.a) * buf[0] + color.r;
				buf[1] = (1-color.a) * buf[1] + color.g;
				buf[2] = (1-color.a) * buf[2] + color.b;
				for(int q = 0; q < 4; q++){
					wei_target[pos + q] = (unsigned char) (buf[q] * 255.0);
				}
				//wei_target[pos + 0] = (unsigned char) (color.r * 255.0);
				//wei_target[pos + 1] = (unsigned char) (color.g * 255.0);
				//wei_target[pos + 2] = (unsigned char) (color.b * 255.0);
				//wei_target[pos + 3] = (unsigned char) (color.a * 255.0);
			}
		}
	}

	else{
		//cout << "traingle mode" << endl;

		x *= sample_rate;
		y *= sample_rate;
		int sx = (int) floor(x), sy = (int) floor(y);

		if(sx < 0 || sx >= wei_target_w) return;
		if(sy < 0 || sy >= wei_target_h) return;

		int pos = 4 * (sx + sy * wei_target_w);


		double buf[4];
		for(int q = 0; q < 4; q++){
			buf[q] = (double) wei_target[pos + q] / 255.0;
		}

		//convert src to pre-mult alpha
		for(int q = 0; q < 3; q++){
			buf[q] *= buf[3];
		}

		//convert dest to pre-mult alpha
		color.r *= color.a; 
		color.g *= color.a;
		color.b *= color.a;

		buf[3] = 1 - (1-color.a) * (1-buf[3]);
		buf[0] = (1-color.a) * buf[0] + color.r;
		buf[1] = (1-color.a) * buf[1] + color.g;
		buf[2] = (1-color.a) * buf[2] + color.b;
		
		//convert result to straight alpha
		for(int q = 0; q < 3; q++){
			buf[q] /= buf[3];
		}

		for(int q = 0; q < 4; q++){
			wei_target[pos + q] = (unsigned char) (buf[q] * 255.0);
		}
		//wei_target[pos + 0] = (unsigned char) (color.r * 255.0);
		//wei_target[pos + 1] = (unsigned char) (color.g * 255.0);
		//wei_target[pos + 2] = (unsigned char) (color.b * 255.0);
		//wei_target[pos + 3] = (unsigned char) (color.a * 255.0); 

	}
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 1: 
  // Implement line rasterization
	//cout << "rasterizing line: colour: " << color << endl;
	double k;
	double start_x_, end_x_, start_y_, end_y_, start_x, start_y, end_x, end_y;
	start_x_ = floor(x0 * sample_rate) * 1.0 / sample_rate;
	//if(x0 - start_x_ > 0.5) start_x_++;
	start_y_ = floor(y0 * sample_rate) * 1.0 / sample_rate;
	//if(y0 - start_y_ > 0.5) start_y_++;
	end_x_ =  floor(x1 * sample_rate) * 1.0 / sample_rate;
	//if(x1 - end_x_ > 0.5) end_x_++;
	end_y_ =  floor(y1 * sample_rate) * 1.0 / sample_rate;
	//if(y1 - end_y_ > 0.5) end_y_++;
	if(start_x_ != end_x_){
		k = 1.0 * (end_y_ - start_y_) / (end_x_ - start_x_);
	}
	else{
		k = 1e14;
	}
	if(k >= -1 && k <= 1){
		if(start_x_ <= end_x_){
			start_x = start_x_;
			start_y = start_y_;
			end_x = end_x_;
			end_y = end_y_;
		}
		else{
			start_x = end_x_;
			start_y = end_y_;
			end_x = start_x_;
			end_y = start_y_;
		}
	}
	else{
		if(start_y_ <= end_y_){
			start_x = start_x_;
			start_y = start_y_;
			end_x = end_x_;
			end_y = end_y_;
		}
		else{
			start_x = end_x_;
			start_y = end_y_;
			end_x = start_x_;
			end_y = start_y_;
		}
	}
	double step = 1.0 / sample_rate;
	if(k <= 1 && k >= 0){
		double x = start_x, y = start_y;
		
		double dx = end_x - start_x, dy = end_y - start_y;
		double e = -0.5;
		for(int i = 0; i <= (dx + 1e-8) / step; i ++){
			//res.push_back(*new Point(x, y));
			rasterize_point(x,y,color);
			x += step;
			e += k;
			if(e >= 0){
				y += step;
				e -= 1.0;
			}
		}
	}
	else if(k > 1){
		double x = start_x, y = start_y;
		double dx = end_x - start_x, dy = end_y - start_y;
		double e = -0.5;
		for(int i = 0; i <= (dy + 1e-8) / step; i ++){
			//res.push_back(*new Point(x, y));
			rasterize_point(x,y,color);
			y += step;
			e += 1 / k;
			if(e >= 0){
				x += step;
				e -= 1.0;
			}
		}
	}
	else if(k >= -1){
		double x = start_x, y = start_y;
		double dx = end_x - start_x, dy = end_y - start_y;
		double e = -0.5;
		for(int i = 0; i <= (dx + 1e-8) / step; i ++){
			//res.push_back(*new Point(x, 2 * start_y - y));
			rasterize_point(x, 2*start_y-y, color);
			x += step;
			e += -k;
			if(e >= 0){
				y += step;
				e -= 1.0;
			}
		}
	}
	else{
		double x = start_x, y = start_y;
		double dx = end_x - start_x, dy = end_y - start_y;
		double e = -0.5;
		for(int i = 0; i <= (dy + 1e-8) / step; i ++){
			//res.push_back(*new Point(2 * start_x - x, y));
			rasterize_point(2*start_x-x, y, color);
			y += step;
			e += -1 / k;
			if(e >= 0){
				x += step;
				e -= 1.0;
			}
		}
	}
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 2: 
  // Implement triangle rasterization
	//cout << "sample_rate: " << sample_rate << endl;
	//cout << "rasterizing traingle: colour: " << color << endl;
	double step = 1.0 / sample_rate;
//cout << "step: " << step << endl;
	int xMin = (int) floor(mn(x0,x1,x2) * sample_rate);
	int xMax = (int) floor(mx(x0,x1,x2) * sample_rate) + 1;
	int yMin = (int) floor(mn(y0,y1,y2) * sample_rate);
	int yMax = (int) floor(mx(y0,y1,y2) * sample_rate) + 1;
	
	for(int i = xMin; i <= xMax; i++){
		for(int j = yMin; j <= yMax; j++){
			if(inside((i+0.5)*step, (j+0.5)*step,x0,y0,x1,y1,x2,y2)){
				rasterize_point(i*step,j*step,color);
			}
		}
	}

}


bool SoftwareRendererImp::inside_ellipse(Vector2D testPoint, Vector2D center, Vector2D radius){
	Vector2D invPoint = invTransform(testPoint);
	double x = invPoint.x, y = invPoint.y;
	double r = center.x, s = center.y;
	double a = radius.x, b = radius.y;
	return (x-r)*(x-r) /a/a + (y-s)*(y-s) /b/b <= 1;
}

void SoftwareRendererImp::rasterize_ellipse(Vector2D center, Vector2D radius, Color cBound, Color cIn){
	double step = 1.0 / sample_rate;
	bool alreadyE = false;
	for(int i = 0; i < target_w * sample_rate; i++){
		bool has = false;
		for(int j = 0; j < target_h * sample_rate; j++){
			Vector2D pt(i * step, j * step);
			bool ise = inside_ellipse(pt, center, radius);
			if(!ise && has) break;
			if(!ise) continue;
			has = true;
			bool onBound = !inside_ellipse(Vector2D(i*step, (j+1)*step), center, radius)
				    || !inside_ellipse(Vector2D(i*step, (j-1)*step), center, radius)
				    || !inside_ellipse(Vector2D((i-1)*step, j*step), center, radius)
				    || !inside_ellipse(Vector2D((i+1)*step, j*step), center, radius);
			if(onBound) rasterize_point(i*step, j*step, cBound);
			else rasterize_point(i*step, j*step, cIn);
		}
		if(has) alreadyE = true;
		if(alreadyE && !has) break;
	}
}


void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task ?: 
  // Implement image rasterization
	//cout << x1-x0 << " " << y1-y0 << endl;
	Sampler2DImp *sdi = new Sampler2DImp(TRILINEAR);
	double step = 1.0 / sample_rate;
	x0 = floor(x0 * sample_rate) * step;
	y0 = floor(y0 * sample_rate) * step;
	x1 = floor(x1 * sample_rate) * step;
	y1 = floor(y1 * sample_rate) * step;
	
	double X0 = x0, Y0 = y0;
	bool printed = false;
	for(X0 = x0; X0 <= x1; X0 += step){
		for(Y0 = y0; Y0 <= y1; Y0 += step){
			//cout << "x=" << X0 << ",y=" << Y0 << endl;
			rasterize_point(X0,Y0,sdi->sample(tex, (X0-x0)/(x1-x0), (Y0-y0)/(y1-y0), 0, step/(x1-x0), step/(y1-y0)));
			if(!printed){
				//tex.print();
				printed = true;
			}
		}
	}
	delete sdi;

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 3: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 3".
	//cout << "sample rate: " << sample_rate << endl;
	for(int i = 0; i < target_w; i++){
		for(int j = 0; j < target_h; j++){
			double temp[4] = {0};
			for(int k = 0; k < sample_rate; k++){
				for(int l = 0; l < sample_rate; l++){
					int da_offset = 4 * ((i * sample_rate + k)  + (j * sample_rate + l) * wei_target_w);
					for(int q = 0; q < 4; q++){
						temp[q] += (double) wei_target[da_offset + q];
					}
				}
			}
			int xiao_offset = 4 * (j * target_w + i);
			for(int q = 0; q < 4; q++){
				render_target[xiao_offset + q] = (unsigned char) (temp[q] / sample_rate / sample_rate);
			}
		}
	}
	
 	return;

}


} // namespace CMU462
