#include "CameraImpl.h"
#include <fstream>
#include <sstream>
//#define __SSE3__
#ifdef __SSE3__
#include "pmmintrin.h"
#endif
using namespace std;

namespace pi{

std::string CameraPinhole::info()
{
    std::stringstream sst;
    sst <<"PinHole:["<<w<<","<<h<<","
       <<fx<<","<<fy<<","<<cx<<","<<cy<<"]";;
    return sst.str();
}

bool CameraPinhole::applyScale(double scale){fx*=scale;fy*=scale;cx*=scale;cy*=scale;return true;}

bool CameraPinhole::isValid(){return w>0&&h>0&&fx!=0&&fy!=0;}

int CameraPinhole::refreshParaments(){if(isValid()){fx_inv=1./fx;fy_inv=1./fy;
        return 0;}else return -1;}

Point2d CameraPinhole::Project(const Point3d& p3d)
{
#ifdef __SSE__
    if(p3d.z==1.)
    {
        __m128d xy = _mm_setr_pd(p3d.x,p3d.y);
        xy=_mm_add_pd(_mm_setr_pd(cx,cy),_mm_mul_pd(xy,(__m128d){fx,fy}));
        return *(Point2d*)&xy;
    }
    else if(p3d.z>0)
    {
        double z_inv=1./p3d.z;
        return Point2d(fx*z_inv*p3d.x+cx,fy*z_inv*p3d.y+cy);
    }
    else return Point2d(-1,-1);
#else
    if(p3d.z==1.)
    {
        return Point2d(fx*p3d.x+cx,fy*p3d.y+cy);
    }
    else if(p3d.z>0)
    {
        double z_inv=1./p3d.z;
        return Point2d(fx*z_inv*p3d.x+cx,fy*z_inv*p3d.y+cy);
    }
    else return Point2d(-1,-1);
#endif
}

Point3d CameraPinhole::UnProject(const Point2d& p2d)
{
    return Point3d((p2d.x-cx)*fx_inv,(p2d.y-cy)*fy_inv,1.);
}


VecParament<double> CameraPinhole::getParameters()
{
    VecParament<double> result(std::vector<double>({(double)w,(double)h,fx,fy,cx,cy}));
    return result;
}

CameraATAN::CameraATAN():fx(0),fy(0),cx(0),cy(0),d(0),fx_inv(0),fy_inv(0){}

CameraATAN::CameraATAN(int _w,int _h,double _fx,double _fy,double _cx,double _cy,double _d)
    :CameraImpl(_w,_h),fx(_fx),fy(_fy),cx(_cx),cy(_cy),d(_d),fx_inv(0),fy_inv(0)
{refreshParaments();}


std::string CameraATAN::info()
{
    std::stringstream sst;
    sst <<"ATAN:["<<w<<","<<h<<","<<fx<<","<<fy
       <<","<<cx<<","<<cy<<","<<d<<"]";
    return sst.str();
}

int CameraATAN::refreshParaments()
{
    if(!isValid()) {
        std::cout<<"Camera not valid!Info:"<<info()<<std::endl;
        return -1;}
    if(fx<1&&fy<1)
    {
        fx*=w;fy*=h;cx*=w;cy*=h;
    }
    if(fx!=0) fx_inv=1./fx;else return -1;
    if(fy!=0) fy_inv=1./fy;else return -2;
    if(d!= 0.0)
    {
        tan2w = 2.0 * tan(d / 2.0);
        tan2w_inv=1.0/tan2w;
        d_inv = 1.0 / d;
        useDistortion = 1.0;
    }
    else
    {
        d_inv = 0.0;
        tan2w = 0.0;
        useDistortion = false;
    }
    return 0;
}

VecParament<double> CameraATAN::getParameters()
{
    VecParament<double> result(std::vector<double>({(double)w,(double)h,fx,fy,cx,cy,d}));
    return result;
}

bool CameraATAN::applyScale(double scale)
{fx*=scale;fy*=scale;cx*=scale;cy*=scale;return true;}

Point2d CameraATAN::Project(const Point3d& p3d)
{
    if(p3d.z<=0) return Point2d(-1,-1);

#ifdef __SSE3__
    if(useDistortion)
    {
        __m128d xy=(__m128d){p3d.x,p3d.y};
        if(p3d.z!=1.)
        {
            xy=_mm_sub_pd(xy,(__m128d){p3d.z,p3d.z});
        }
        __m128d xy2=_mm_mul_pd(xy,xy);

         xy2=_mm_hadd_pd(xy2,xy2);
         xy2=_mm_sqrt_pd(xy2);
        double r=((Point2d*)&xy2)->x;
        if(r < 0.001 || d == 0.0)
            r=1.0;
        else
            r=(d_inv* atan(r * tan2w) / r);
        xy=_mm_mul_pd((__m128d){fx,fy},xy);
        xy=_mm_mul_pd(xy,(__m128d){r,r});
        xy=_mm_add_pd(xy,(__m128d){cx,cy});
        return *(Point2d*)&xy;
    }
    else
    {
        if(p3d.z==1.)
        {
            __m128d xy = _mm_setr_pd(p3d.x,p3d.y);
            xy=_mm_add_pd(_mm_setr_pd(cx,cy),_mm_mul_pd(xy,(__m128d){fx,fy}));
            return *(Point2d*)&xy;
        }
        else if(p3d.z>0)
        {
            double z_inv=1./p3d.z;
            return Point2d(fx*z_inv*p3d.x+cx,fy*z_inv*p3d.y+cy);
        }
    }
#else
    if(useDistortion)
    {
        double X=p3d.x,Y=p3d.y;
        if(p3d.z!=1.)
        {
            double z_inv=1./p3d.z;
            X*=z_inv;Y*=z_inv;
        }
        double r= sqrt(X*X+Y*Y);
        if(r < 0.001 || d == 0.0)
            r= 1.0;
        else
            r=(d_inv* atan(r * tan2w) / r);

        return Point2d(cx + fx * r * X,cy + fy * r * Y);
    }
    else
    {
        if(p3d.z==1.)
        {
            return Point2d(fx*p3d.x+cx,fy*p3d.y+cy);
        }
        else
        {
            double z_inv=1./p3d.z;
            return Point2d(fx*z_inv*p3d.x+cx,fy*z_inv*p3d.y+cy);
        }
    }
#endif
    return Point2d(-1,-1);// let compiler happy
}

Point3d CameraATAN::UnProject(const Point2d& p2d)
{
    if(useDistortion)
    {
        Point3d result((p2d.x-cx)*fx_inv,(p2d.y-cy)*fy_inv,1.);
        double r = sqrt(result.x*result.x+result.y*result.y);

        if(d != 0.0&& r >0.01)
        {
            r = ( tan(r * d)*tan2w_inv )/r;//;
            result.x=result.x*r;
            result.y=result.y*r;
        }
        return result;
    }
    else return Point3d((p2d.x-cx)*fx_inv,(p2d.y-cy)*fy_inv,1.);
}


std::string CameraOpenCV::info()
{
    std::stringstream sst;
    sst<<"OpenCV:["<<w<<","<<h<<","<<fx<<","<<fy<<","<<cx<<","<<cy<<","
                <<k1<<","<<k2<<","<<p1<<","<<p2<<","<<k3<<"]";
    return sst.str();
}

VecParament<double> CameraOpenCV::getParameters()
{
    VecParament<double> result(std::vector<double>({(double)w,(double)h,fx,fy,cx,cy,k1,k2,p1,p2,k3}));
    return result;
}

bool CameraOpenCV::applyScale(double scale){fx*=scale;fy*=scale;cx*=scale;cy*=scale;return true;}

int CameraOpenCV::refreshParaments()
{
    if(!isValid()) {
        std::cout<<"Camera not valid!Info:"<<info()<<std::endl;
        return -1;}
    if(fx!=0) fx_inv=1./fx;else return -1;
    if(fy!=0) fy_inv=1./fy;else return -2;
    return 0;
}

Point2d CameraOpenCV::Project(const Point3d& p3d)
{
    if(p3d.z<=0) return Point2d(-1,-1);
    double X=p3d.x,Y=p3d.y;
    if(p3d.z!=1.)
    {
        double z_inv=1./p3d.z;
        X*=z_inv;Y*=z_inv;
    }

    double r2,r4,r6,X1,Y1,X2,Y2,xy2;
    X2=X*X;
    Y2=Y*Y;
    r2= X2+Y2;
    r4=r2*r2;
    r6=r2*r4;
    xy2=X*Y*2.0;
    X1= X*(1.+k1*r2 + k2*r4+k3*r6) + xy2*p1 + p2*(r2 + 2.0*X2);
    Y1= Y*(1.+k1*r2 + k2*r4+k3*r6) + xy2*p2 + p1*(r2 + 2.0*Y2);

    return Point2d(cx + fx * X1,cy + fy * Y1);
}

Point3d CameraOpenCV::UnProject(const Point2d& p2d)
{
    double x = (p2d.x - cx)*fx_inv;
    double y = (p2d.y - cy)*fy_inv;

    // compensate tilt distortion
    double x0 = x ;
    double y0 = y ;
    // compensate distortion iteratively
    for(int j = 0; j < 5; j++ )
    {
        double r2 = x*x + y*y;
        double icdist = (1)/(1 + ((k3*r2 + k2)*r2 + k1)*r2);
        double deltaX = 2*p1*x*y + p2*(r2 + 2*x*x);
        double deltaY = p1*(r2 + 2*y*y) + 2*p2*x*y;
        x = (x0 - deltaX)*icdist;
        y = (y0 - deltaY)*icdist;
    }
    return pi::Point3d(x,y,1);
}

CameraOCAM::CameraOCAM(const std::string& filename)
    :length_pol(0),length_invpol(0)
{
    {
        std::ifstream ifs(filename.c_str());
        if(!ifs.is_open())
        {
            printf("File %s cannot be opened\n", filename.c_str());
            return;
        }

        std::string line;
        if(!std::getline(ifs,line)) return;
        ifs>>length_pol;
        for(int i=0;i<length_pol;i++)
        {
            ifs>>pol[i];
        }
        if(!std::getline(ifs,line)) return;
        ifs>>length_invpol;
        for(int i=0;i<length_invpol;i++)
        {
            ifs>>invpol[i];
        }
        if(!std::getline(ifs,line)) return;
        ifs>>cx>>cy;
        if(!std::getline(ifs,line)) return;
        ifs>>c>>d>>e;
        if(!std::getline(ifs,line)) return;
        ifs>>h>>w;
    }
}

VecParament<double> CameraOCAM::getParameters()
{
    VecParament<double> result(std::vector<double>({(double)w,(double)h,cx,cy,c,d,e,(double)length_pol,(double)length_invpol}));

    for(int i=0;i<length_pol;i++) result.data.push_back(pol[i]);
    for(int i=0;i<length_invpol;i++) result.data.push_back(invpol[i]);
    return result;
}

std::string CameraOCAM::info()
{
    std::stringstream sst;
    sst<<"OCAM:["<<w<<" "<<h<<" "
      <<cx<<" "<<cy<<" "<<c<<" "<<d<<" "<<e<<"]";
#if 0
    sst<<"pol=";
    for(int i=0;i<length_pol;i++) sst<<" "<<pol[i];
    sst<<"\ninv_pol=";
    for(int i=0;i<length_invpol;i++) sst<<" "<<invpol[i];
#endif
    return sst.str();
}

Point2d CameraOCAM::Project(const Point3d& point)
{
    Point2d result;
    double norm        = sqrt(point.x*point.x + point.y*point.y);
    double theta       = atan(-point.z/norm);
    double t, t_i;
    double rho, x, y;
    double invnorm;
    int i;

    if (norm != 0)
    {
        invnorm = 1/norm;
        t  = theta;
        rho = invpol[0];
        t_i = 1;
        //            std::cerr<<"i="<<0<<",invpol="<<invpol[0]<<",rho="<<rho<<std::endl;

        for (i = 1; i < length_invpol; i++)
        {
            t_i *= t;
            rho += t_i*invpol[i];
            //                std::cerr<<"i="<<i<<",invpol="<<invpol[i]<<",rho="<<rho<<std::endl;
        }
        x = point.x*invnorm*rho;
        y = point.y*invnorm*rho;

        result.y = y*c + x*d + cy;
        result.x = y*e + x   + cx;
    }
    else
    {
        result.x=cx;
        result.y=cy;
    }
    return result;
}

Point3d CameraOCAM::UnProject(const Point2d& point)
{
    double invdet  = 1/(c-d*e); // 1/det(A), where A = [c,d;e,1] as in the Matlab file
    double yp = invdet*(    (point.y - cy) - d*(point.x - cx) );
    double xp = invdet*( -e*(point.y - cy) + c*(point.x - cx) );

    double r   = sqrt(  xp*xp + yp*yp ); //distance [pixels] of  the point from the image center
    double zp  = pol[0];
    double r_i = 1;
    int i;

    for (i = 1; i < length_pol; i++)
    {
        r_i *= r;
        zp  += r_i*pol[i];
    }

    //normalize to unit norm
    double invnorm = 1/sqrt( xp*xp + yp*yp + zp*zp );
    return Point3d(invnorm*xp,invnorm*yp,-invnorm*zp);
}

}
