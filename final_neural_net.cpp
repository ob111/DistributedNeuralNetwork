//final-neural-net.cpp

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <iomanip>
using namespace std;
/* copied from mpbench */
#define TIMER_CLEAR     (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec = 0)
#define TIMER_START     gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_ELAPSED   (double) (tv2.tv_usec-tv1.tv_usec)/1000000.0+(tv2.tv_sec-tv1.tv_sec)
#define TIMER_STOP      gettimeofday(&tv2, (struct timezone*)0)
struct timeval tv1,tv2;

void get_num_threads(int argc,char *argv[],int *input) {
   if(argc!=2) {
       cout<<"usage:  ./neural_net <numthreads>" << endl;
       exit(1);
   }
   else {
       if (argc == 2) {
           *input = atoi(argv[1]);
       }

   }
}
class Neuron;
struct Conn
{
	double w;
	double deltaW;
};

typedef std::vector<Neuron> Layer;

class Data{
private:
    std::ifstream trainingFile;
public:
  //Reads in XOR data from file.
  Data(const std::string filename){
      trainingFile.open(filename.c_str());
  }
  bool endFile(void){
     return trainingFile.eof();
   }
  unsigned getInput(std::vector<double> &input){
      input.clear();

      std::string line;
      getline(trainingFile, line);
      std::stringstream ss(line);

      std::string label;
      ss>> label;
      if (label.compare("in:") == 0) {
          double oneValue;
          while (ss >> oneValue) {
              input.push_back(oneValue);
          }
      }

      return input.size();
  }
  unsigned targetOutputs(std::vector<double> &vals){
      vals.clear();

      std::string line;
      getline(trainingFile, line);
      std::stringstream ss(line);

      std::string label;
      ss>> label;
      if (label.compare("out:") == 0) {
          double oneValue;
          while (ss >> oneValue) {
              vals.push_back(oneValue);
          }
      }

      return vals.size();
  }

};


class Neuron{
public:
  Neuron(unsigned numOutputs, unsigned myIndex){
  	for (unsigned c = 0; c < numOutputs; ++c) {
  		m_outputWeights.push_back(Conn());
  		m_outputWeights.back().w = randomWeight();
  	}

  	m_myIndex = myIndex;
  }
  void setOutputVal(double val) {
    m_outputVal = val;
  }

  double getOutputVal(void) const {
    return m_outputVal;
  }

  void feedForward(const Layer &prevLayer){
  	double sum = 0.0;

  	// sum the previous layer's outputs (which are our inputs)
  	// include the bias node from previous layer

  	for (unsigned n = 0; n < prevLayer.size(); ++n) {
  		sum += prevLayer[n].getOutputVal() *
  				prevLayer[n].m_outputWeights[m_myIndex].w;
  	}

  	m_outputVal = Neuron::transferFunction(sum);
  }

  void calcOutputGradients(double targetVal){
  	double delta = targetVal - m_outputVal;
  	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputVal);
  }

  void calcHiddenGradients(const Layer &nextLayer){
  	double dow = sumDOW(nextLayer);
  	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputVal);
  }

  void updateInputWeights(Layer &prevLayer){
  	for (unsigned n = 0; n < prevLayer.size(); ++n) {
  		Neuron &neuron = prevLayer[n];
  		double oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaW;

  		double newDeltaWeight =
  				// individual input, magnified by the gradient and train rate:
  				eta
  				* neuron.getOutputVal()
  				* m_gradient
  				// also add momentum = a fraction of the previous delta weight
  				+ alpha
  				* oldDeltaWeight;

  		neuron.m_outputWeights[m_myIndex].deltaW = newDeltaWeight;
  		neuron.m_outputWeights[m_myIndex].w += newDeltaWeight;
  	}
  }

private:
  static double eta; // [0..1] overall net training rate
	static double alpha; // [0..n] multiplier of last weight change (momentum)
	static double transferFunction(double x){
  	return tanh(x);
  }
	static double transferFunctionDerivative(double x){
  	// tanh derivative
  	return 1.0 - x * x;
  }
	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	double sumDOW(const Layer &nextLayer){
  	double sum = 0.0;

  	// sum our contributions of the errors at the nodes we feed

  	for (unsigned n = 0; n < nextLayer.size() - 1; ++n) {
  		sum += m_outputWeights[n].w * nextLayer[n].m_gradient;
  	}

  	return sum;
  }
	double m_outputVal;
	std::vector<Conn> m_outputWeights;
	unsigned m_myIndex;
	double m_gradient;
};
double Neuron::eta = 0.15; // overall net learning rate
double Neuron::alpha = 0.5; // momentum, multiplier of last deltaWeight

class Net{
private:
  std::vector<Layer> m_layers; //m_layers[layerNum][neuronNum]
  double m_error;
  double m_recentAverageError;
  double m_recentAverageSmoothingFactor;
public:
  Net(const std::vector<unsigned> &topology)
  {
  	unsigned numLayers = topology.size();
  	for (unsigned layerNum = 0; layerNum < numLayers; ++layerNum) {
  		m_layers.push_back(Layer()); //Adding object of type layer (or adding layers)
  		unsigned numOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1]; //creating number of outputs for current layer

  		// we have made a new layer, now fill it with neurons and
  		// add a bias neuron to the layer:
  		for (unsigned neuronNum = 0; neuronNum <= topology[layerNum]; ++neuronNum) {
  			m_layers.back().push_back(Neuron(numOutputs, neuronNum));
  			// std::cout << "Made a Neuron!" << std::endl;
  		}
  		// force the bias node's output value to 1.0. It's the last neuron created above
  		m_layers.back().back().setOutputVal(1.0);
  	}
  }

  void feedForward(const std::vector<double> &inputVals)
  {
  	assert(inputVals.size() == m_layers[0].size() - 1);

  	// assign (latch) the input values into the input neurons
  	for (unsigned i = 0; i < inputVals.size(); ++i) {
  		m_layers[0][i].setOutputVal(inputVals[i]);
  	}

  	// forward propagate
  	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum) {
  		Layer &prevLayer = m_layers[layerNum - 1];
  		for (unsigned n = 0; n < m_layers[layerNum].size() - 1; ++n) {
  			m_layers[layerNum][n].feedForward(prevLayer);
  		}
  	}
  }

  void backProp(const std::vector<double> &targetVals)
  {
  	// calc overall net error (rms of output neuron errors)

  	Layer &outputLayer = m_layers.back();
  	m_error = 0.0;

  	for (unsigned n = 0; n < outputLayer.size() - 1; ++n) {
  		double delta = targetVals[n] - outputLayer[n].getOutputVal();
  		m_error += delta * delta;
  	}
  	m_error /= outputLayer.size() - 1;
  	m_error = sqrt(m_error);  // RMS

  	// implement a recent avg measurement
  	m_recentAverageError =
  			(m_recentAverageError * m_recentAverageSmoothingFactor + m_error)/ (m_recentAverageSmoothingFactor + 1.0);

  	// calc output layer gradients

  	for (unsigned n = 0; n < outputLayer.size() - 1; ++n) {
  		outputLayer[n].calcOutputGradients(targetVals[n]);
  	}	// calc gradients on hidden layers

    	for (unsigned layerNum = m_layers.size() - 2; layerNum > 0; --layerNum) {
    		Layer &hiddenLayer = m_layers[layerNum];
    		Layer &nextLayer = m_layers[layerNum + 1];

    		for (unsigned n = 0; n < hiddenLayer.size(); ++n) {
    			hiddenLayer[n].calcHiddenGradients(nextLayer);
    		}
    	}

    	// for all layers from outputs to first hidden layer,
    	// update connection weights

    	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; --layerNum) {
    		Layer &layer = m_layers[layerNum];
    		Layer &prevLayer = m_layers[layerNum - 1];

    		for (unsigned n = 0; n < layer.size() - 1; ++n) {
    			layer[n].updateInputWeights(prevLayer);
    		}
    	}
    }

    void getResults(std::vector<double> &resultVals) const
    {
    	resultVals.clear();

    	for (unsigned n = 0; n < m_layers.back().size() - 1; ++n)
    	{
    		resultVals.push_back(m_layers.back()[n].getOutputVal());
    	}
    }

    double getRecentAverageError(void) const { return m_recentAverageError; }

};
void showVectorVals(std::string label, std::vector<double> &v)
{
    std::cout << label << " ";
    for (unsigned i = 0; i < v.size(); ++i) {
        std::cout << v[i] << " ";
    }

    std::cout << std::endl;
}



int main(int argc, char *argv[]){
  int input;
 get_num_threads(argc,argv,&input);
 int nthreads = atoi(argv[1]);
//    omp_set_dynamic(0);
//    omp_set_num_threads(nthreads);
    printf("NumThreads before loop: %d \n",nthreads);
  Data data("data.txt");
//    TrainingData trainData("train-poker.txt");
  // std::ifstream myfile;
  // std::string file = "data.txt";
  // myfile.open(file.c_str());

  // e.g., { 3, 2, 1 }
  std::vector<unsigned> topology;
  // topology is set to 2 4 4 1
  topology.push_back(2);
  topology.push_back(4);
  topology.push_back(4);
  topology.push_back(1);

  Net myNet(topology);
  std::vector<double> inputVals, targetVals, resultVals;
  int trainingPass = 0;

  TIMER_CLEAR;
  TIMER_START;
#pragma omp parallel num_threads(nthreads)  // shared(a,b,c)
    {
        int tid = omp_get_thread_num();
        printf("NumThreads in loop: %d \n",nthreads);
      while (!data.endFile()) {
          ++trainingPass;
          std::cout << std::endl << "Pass " << trainingPass << endl;
//          printf("Thread ID = %d \n",tid);

          // Get new input data and feed it forward:
          if (data.getInput(inputVals) != topology[0]) {
            // printf("Debugger \n" );
              break;

          }

//          showVectorVals(": Inputs:", inputVals);

          myNet.feedForward(inputVals);


          // Collect the net's actual output results:
          myNet.getResults(resultVals);
//          showVectorVals("Outputs:", resultVals);

          // Train the net what the outputs should have been:
          data.targetOutputs(targetVals);
//          showVectorVals("Targets:", targetVals);
//          assert(targetVals.size() == topology.back());


          myNet.backProp(targetVals);

          // Report how well the training is working, average over recent samples:
//          std::cout << "Net recent average error: " << myNet.getRecentAverageError() << std::endl;
      }
    }
  TIMER_STOP;
//}
  std::cout << std::endl << "Done" << std::endl;
  std::cout << std::endl << "Time= "<<std::setprecision(8)<<TIMER_ELAPSED << " seconds"<< std::endl;
  for(int i = 0; i < topology.size(); i++){
     std::cout << "Topology: " << topology[i] << std::endl;
   }

  return 0;
}
