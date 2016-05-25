#ifndef QGSHILLSHADERENDERER_H
#define QGSHILLSHADERENDERER_H


#include "qgsrasterrenderer.h"

class QgsRasterBlock;
class QgsRectangle;
class QgsRasterInterface;


class CORE_EXPORT QgsHillshadeRenderer : public QgsRasterRenderer
{
public:
  QgsHillshadeRenderer(QgsRasterInterface* input, int band , double lightAzimuth, double lightAngle);

  QgsHillshadeRenderer * clone() const override;

  static QgsRasterRenderer* create( const QDomElement& elem, QgsRasterInterface* input );

  QgsRasterBlock *block( int bandNo, QgsRectangle  const & extent, int width, int height ) override;

  QList<int> usesBands() const override;

    /** Returns the band used by the renderer
     * @note added in QGIS 2.7
     */
    int band() const { return mBand; }

    /** Sets the band used by the renderer.
     * @see band
     * @note added in QGIS 2.10
     */
    void setBand( int bandNo );

    void setAzimuth( double angle ) { mLightAzimuth = angle; }
    void setAngle( double sun ) { mLightAngle = sun; }
    void setZFactor( double z ) { mZFactor = z; }

 private:
    int mBand;

    /** The nodata value of the input layer*/
    double mInputNodataValue;
    double mOutputNodataValue;

    double mZFactor;
  double mLightAngle;
  double mLightAzimuth;

    /** Calculates the first order derivative in x-direction according to Horn (1981)*/
    float calcFirstDerX(double x11, double x21, double x31, double x12, double x22, double x32, double x13, double x23, double x33 , float mCellSize);

    /** Calculates the first order derivative in y-direction according to Horn (1981)*/
    float calcFirstDerY(double x11, double x21, double x31, double x12, double x22, double x32, double x13, double x23, double x33 , float mCellSize);
};

#endif // QGSHILLSHADERENDERER_H
