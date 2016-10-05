%module esn

%{
#include "esn/network.h"
#include "esn/trainer.h"
%}

%include <std_shared_ptr.i>
%shared_ptr(ESN::Network)
%shared_ptr(ESN::Trainer)

%include <std_vector.i>
%template(Vector) std::vector<float>;

%include "esn/export.h"
%include "esn/network.h"
%include "esn/trainer.h"