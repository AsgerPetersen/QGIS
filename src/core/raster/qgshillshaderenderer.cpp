#include <QColor>

#include "qgshillshaderenderer.h"

#include "qgsrasterinterface.h"
#include "qgsrasterblock.h"
#include "qgsrectangle.h"


QgsHillshadeRenderer::QgsHillshadeRenderer(QgsRasterInterface *input, int band, double lightAzimuth, double lightAngle):
    QgsRasterRenderer( input, "hillshade" ),
    mBand( 1 ),
    mInputNodataValue( 0 ),
    mOutputNodataValue( 0 ),
    mZFactor( 1 ),
    mLightAngle( lightAngle ),
    mLightAzimuth( lightAzimuth )
{
  QgsDebugMsg( QString("%1").arg( mLightAngle ));
  QgsDebugMsg( QString("%1").arg( mLightAzimuth ));
}

QgsHillshadeRenderer *QgsHillshadeRenderer::clone() const
{
  QgsHillshadeRenderer* r = new QgsHillshadeRenderer( nullptr, mBand, mLightAzimuth, mLightAngle );
  r->setZFactor( mZFactor );
  return r;
}

QgsRasterRenderer *QgsHillshadeRenderer::create(const QDomElement &elem, QgsRasterInterface *input)
{
  if ( elem.isNull() )
  {
    return nullptr;
  }

  int band = elem.attribute( "band", "-1" ).toInt();
  QgsHillshadeRenderer* r = new QgsHillshadeRenderer( input, band, 300, 30 );
  return r;
}

QgsRasterBlock *QgsHillshadeRenderer::block(int bandNo, const QgsRectangle &extent, int width, int height)
{
  Q_UNUSED( bandNo );
  QgsDebugMsg("BLOCK!");
  QgsRasterBlock *outputBlock = new QgsRasterBlock();
  if ( !mInput )
  {
    QgsDebugMsg("No input raster!");
    return outputBlock;
  }

  QgsRasterBlock *inputBlock = mInput->block( mBand, extent, width, height );

  if ( !inputBlock || inputBlock->isEmpty() )
  {
    QgsDebugMsg( "No raster data!" );
    delete inputBlock;
    return outputBlock;
  }

  if ( !outputBlock->reset( QGis::ARGB32_Premultiplied, width, height ) )
  {
    delete inputBlock;
    return outputBlock;
  }

  mInputNodataValue = inputBlock->noDataValue();
  QRgb myDefaultColor = qRgb(255, 0,0 );
//  QRgb myDefaultColor = NODATA_COLOR;
  qgssize xSize = (qgssize)width*height;
  for ( qgssize i = 0; i < height; i++ )
    {

      for ( qgssize j = 0; j < width; j++)
        {

          if ( inputBlock->isNoData( i, j ) )
          {
            outputBlock->setColor( i, j, myDefaultColor );
            continue;
          }

          qgssize iUp, iDown, jLeft, jRight;
          if( i == 0 )
          {
            iUp = i;
            iDown = i + 1;
          }
          else if( i < height - 1 )
          {
            iUp = i - 1;
            iDown = i + 1;
          }
          else
          {
              iUp = i - 1;
              iDown = i;
          }

          if( j == 0)
          {
              jLeft = j;
              jRight = j + 1;
          }
          else if( j < width - 1)
          {
              jLeft = j - 1;
              jRight = j + 1;
          }
          else
          {
              jLeft = j - 1;
              jRight = j;
          }

          double x11;
          double x21;
          double x31;
          double x12;
          double x22; // Working cell
          double x32;
          double x13;
          double x23;
          double x33;

          // This is center cell. It is not nodata. Use this in place of nodata neighbors
          x22 = inputBlock->value(i, j);

        x11 = inputBlock->isNoData(iUp, jLeft )  ? x22 : inputBlock->value(iUp, jLeft);
        x21 = inputBlock->isNoData(i, jLeft)     ? x22 : inputBlock->value(i, jLeft);
        x31 = inputBlock->isNoData(iDown, jLeft) ? x22 : inputBlock->value(iDown, jLeft);

        x12 = inputBlock->isNoData(iUp, j)       ? x22 : inputBlock->value(iUp, j);
        // x22
        x32 = inputBlock->isNoData(iDown, j)     ? x22 : inputBlock->value(iDown, j);

        x13 = inputBlock->isNoData(iUp, jRight)   ? x22 :inputBlock->value(iUp, jRight);
        x23 = inputBlock->isNoData(i, jRight)     ? x22 : inputBlock->value(i, jRight);
        x33 = inputBlock->isNoData(iDown, jRight) ? x22 : inputBlock->value(iDown, jRight);

      float resolution = extent.width() / float(width);
  double derX = calcFirstDerX( x11, x21, x31, x12, x22, x32, x13, x23, x33, resolution );
      resolution = extent.height() / float(height);
  double derY = calcFirstDerY( x11, x21, x31, x12, x22, x32, x13, x23, x33, resolution );

  float zenith_rad = qMax(0.0, 90 - mLightAngle) * M_PI / 180.0;
  float slope_rad = atan( mZFactor * sqrt( derX * derX + derY * derY ) );
  float azimuth_rad = -1 * mLightAzimuth * M_PI / 180.0;
  double aspect_rad = 0;

  if ( derX != 0)
    {
        aspect_rad = atan2( derX, -derY );
        if ( aspect_rad < 0 )
          {
            aspect_rad = 2 * M_PI + aspect_rad;
          }
    }

  if ( derX == 0)
    {
      if ( derY > 0 )
        {
          aspect_rad = M_PI / 2;
        }
      else if ( derY < 0 )
        {
           aspect_rad = 2 * M_PI - M_PI/2;
        }
      else
        aspect_rad = aspect_rad;
    }

  double colorvalue = qMax( 0.0, 255.0 * ( ( cos( zenith_rad ) * cos( slope_rad ) ) +
                                           ( sin( zenith_rad ) * sin( slope_rad ) * cos( azimuth_rad - aspect_rad ) ) ) );
  outputBlock->setColor( i, j, qRgb( colorvalue, colorvalue, colorvalue));
   }
    }
  return outputBlock;
}

QList<int> QgsHillshadeRenderer::usesBands() const
{
  QList<int> bandList;
  if ( mBand != -1 )
  {
    bandList << mBand;
  }
  return bandList;

}

void QgsHillshadeRenderer::setBand(int bandNo)
{
  if ( bandNo > mInput->bandCount() || bandNo <= 0 )
  {
    return;
  }
  mBand = bandNo;
}


float QgsHillshadeRenderer::calcFirstDerX( double x11, double x21, double x31, double x12, double x22, double x32, double x13, double x23, double x33, float mCellSize )
{
  //the basic formula would be simple, but we need to test for nodata values...
  return ((x13 + x23 + x23 + x33) - (x11 + x21 + x21 + x31)) / (8 * mCellSize);
//  return (( (*x31 - *x11) + 2 * (*x32 - *x12) + (*x33 - *x13) ) / (8 * mCellSizeX));

  int weight = 0;
  double sum = 0;

  //first row
  if ( x31 != mInputNodataValue && x11 != mInputNodataValue ) //the normal case
  {
    sum += ( x13 - x11 );
    weight += 2;
  }
  else if ( x31 == mInputNodataValue && x11 != mInputNodataValue && x21 != mInputNodataValue ) //probably 3x3 window is at the border
  {
    sum += ( x21 - x11 );
    weight += 1;
  }
  else if ( x11 == mInputNodataValue && x31 != mInputNodataValue && x21 != mInputNodataValue ) //probably 3x3 window is at the border
  {
    sum += ( x31 - x21 );
    weight += 1;
  }

  //second row
  if ( x32 != mInputNodataValue && x12 != mInputNodataValue ) //the normal case
  {
    sum += 2 * ( x32 - x12 );
    weight += 4;
  }
  else if ( x32 == mInputNodataValue && x12 != mInputNodataValue && x22 != mInputNodataValue )
  {
    sum += 2 * ( x22 - x12 );
    weight += 2;
  }
  else if ( x12 == mInputNodataValue && x32 != mInputNodataValue && x22 != mInputNodataValue )
  {
    sum += 2 * ( x32 - x22 );
    weight += 2;
  }

  //third row
  if ( x33 != mInputNodataValue && x13 != mInputNodataValue ) //the normal case
  {
    sum += ( x33 - x13 );
    weight += 2;
  }
  else if ( x33 == mInputNodataValue && x13 != mInputNodataValue && x23 != mInputNodataValue )
  {
    sum += ( x23 - x13 );
    weight += 1;
  }
  else if ( x13 == mInputNodataValue && x33 != mInputNodataValue && x23 != mInputNodataValue )
  {
    sum += ( x33 - x23 );
    weight += 1;
  }

  if ( weight == 0 )
  {
    return mOutputNodataValue;
  }

  return sum / (weight * 0.5 );
}

float QgsHillshadeRenderer::calcFirstDerY( double x11, double x21, double x31, double x12, double x22, double x32, double x13, double x23, double x33, float mCellSize )
{
  //the basic formula would be simple, but we need to test for nodata values...
  return (( x31 + x32 + x32 + x33) - ( x11 + x12 + x12 + x13)) / (8 * -mCellSize);
//   return (((x11 - x13) + 2 * (x21 - x23) + (x31 - x33)) / ( 8 * mCellSizeY));

  double sum = 0;
  int weight = 0;

  //first row
  if ( x11 != mInputNodataValue && x13 != mInputNodataValue ) //normal case
  {
    sum += ( x11 - x13 );
    weight += 2;
  }
  else if ( x11 == mInputNodataValue && x13 != mInputNodataValue && x12 != mInputNodataValue )
  {
    sum += ( x12 - x13 );
    weight += 1;
  }
  else if ( x31 == mInputNodataValue && x11 != mInputNodataValue && x12 != mInputNodataValue )
  {
    sum += ( x11 - x12 );
    weight += 1;
  }

  //second row
  if ( x21 != mInputNodataValue && x23 != mInputNodataValue )
  {
    sum += 2 * ( x21 - x23 );
    weight += 4;
  }
  else if ( x21 == mInputNodataValue && x23 != mInputNodataValue && x22 != mInputNodataValue )
  {
    sum += 2 * ( x22 - x23 );
    weight += 2;
  }
  else if ( x23 == mInputNodataValue && x21 != mInputNodataValue && x22 != mInputNodataValue )
  {
    sum += 2 * ( x21 - x22 );
    weight += 2;
  }

  //third row
  if ( x31 != mInputNodataValue && x33 != mInputNodataValue )
  {
    sum += ( x31 - x33 );
    weight += 2;
  }
  else if ( x31 == mInputNodataValue && x33 != mInputNodataValue && x32 != mInputNodataValue )
  {
    sum += ( x32 - x33 );
    weight += 1;
  }
  else if ( x33 == mInputNodataValue && x31 != mInputNodataValue && x32 != mInputNodataValue )
  {
    sum += ( x31 - x32 );
    weight += 1;
  }

  if ( weight == 0 )
  {
    return mOutputNodataValue;
  }

  return sum / (weight * -0.5 );
}




