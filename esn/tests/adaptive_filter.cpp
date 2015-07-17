#include <cmath>
#include <Eigen/Dense>
#include <gtest/gtest.h>

class ReferenceFilter
{
public:
    ReferenceFilter( unsigned inputCount, unsigned outputCount )
        : mW( Eigen::MatrixXf::Random( outputCount, inputCount ) )
    {}

    Eigen::VectorXf operator()( Eigen::VectorXf inputs )
    {
        return mW * inputs;
    }

    Eigen::MatrixXf mW;
};

TEST( AdaptiveFilter, LMS )
{
    const unsigned kInputCount = 5;
    const unsigned kOutputCount = 3;
    const unsigned kSampleCount = 100000;
    const float kMaxAmplitude = 1.0f;
    const float kMaxFrequency = 10.0f;
    const float kStep = 0.1f * 1.0f / kMaxFrequency;

    ReferenceFilter referenceFilter( kInputCount, kOutputCount );
    Eigen::VectorXf input( kInputCount );
    Eigen::VectorXf A = kMaxAmplitude / 2.0f *
        ( Eigen::VectorXf::Random( kInputCount ).array() + 1.0f );
    Eigen::VectorXf omega = kMaxFrequency / 2.0f *
        ( Eigen::VectorXf::Random( kInputCount ).array() + 1.0f );

    Eigen::MatrixXf Wout = Eigen::MatrixXf::Random(
        kOutputCount, kInputCount );

    for ( int i = 1; i < kSampleCount; ++ i )
    {
        float t = kStep * i;
        input = A.array() * ( omega.array() * t ).unaryExpr(
            std::ptr_fun< float, float >( std::sin ) );
        Eigen::VectorXf referenceOutput = referenceFilter( input );
        Eigen::VectorXf currentOutput = Wout * input;
        Eigen::VectorXf error = referenceOutput - currentOutput;
        Wout += ( 0.1f * error * input.transpose() / input.squaredNorm() );
    }
}