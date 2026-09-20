#include <iostream>
#include <random>
#include <fstream>

#define DISTANCE_TEST_HEADER_FILENAME "root_distance_tests.h"

float get_random_float()
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<> dis(-1, 1); // range [-1, 1)
    return dis(e);
}

int get_num_tests(int order)
{
    /*
    // Reverse of the Sigmoid function shifted horizontaly:
    // f(x) = ymin + (ymax - ymin) / (1 + e^(k(x-x0)),
    // where :
    //      [ymin,ymax] is the range of examples,
    //      x0 is the function has declined by 50%
    //      k defines the steepness of the declining curve
    // So for ymax=10, ymin=3, x0=20 and k=0.3, the function becomes:
    // f(x) = 3 + 7 / (1 + e^0.3(x-20))
    // ref:check f\left(x\right)=3\ +\ \frac{7}{\left(1+e^{0.3\left(x-20\right)}\right)} in https://www.desmos.com/calculator
    */
    return static_cast<int>(std::round(3 + 7 / (1 + std::exp( 0.3 * (order-20)))));
}

void generate_root_distance_tests(int argc, char* argv[])
{
    std::string filename(DISTANCE_TEST_HEADER_FILENAME);
    if (argc==2){
        filename = std::string(argv[1]) + "/" + filename;
    }

    std::ofstream outStream(filename);

    srand (static_cast <unsigned> (time(0)));

    std::string rootTypes[2] = {"pole", "zero"};
    std::string dts[2] = {"real","complex"};

    for (int typeidx=0; typeidx<2;++typeidx)
    {   
        if (typeidx == 0)
            outStream << "#define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_POLES_XLIST\\"<<'\n';
        else    
            outStream << "#define COEFFICIENTS_TO_ROOTS_DISTANCE_TEST_ZEROS_XLIST\\"<<'\n';

        // for each test
        for (int order=1;order<=32;++order)
        {
            int orderCtr=0;
            int num_tests = get_num_tests(order);
            std::cout<<"Order "<<order<<" generates "<<num_tests<<" tests."<<std::endl;
            for (int sample=0; sample<num_tests; sample++ )
            {

                std::string dt = dts[(order+sample)%2];
            
                std::string testLine = "X(\"distance} ";
                testLine+= std::to_string(order);
                int norder = (rootTypes[typeidx].compare("pole")) ? order : -order;
                testLine+= (": "+dt+" "+rootTypes[typeidx]+" order "+std::to_string(++orderCtr) +"\", { {" + std::to_string(norder) +","); 

                float x,y;
                if (dt.compare("complex")){
                    y = 0.0f;
                    x= get_random_float();
                }
                else{
                    y = get_random_float();
                    x= get_random_float();
                }
                testLine += (std::to_string(x) + "," + std::to_string(y) + "} } )\\");
                outStream << '\t' << testLine << '\n';
            }
        }
        outStream<<"\n\n";
    }
    outStream.close();
}


int main(int argc, char* argv[]){

    generate_root_distance_tests(argc, argv);

}