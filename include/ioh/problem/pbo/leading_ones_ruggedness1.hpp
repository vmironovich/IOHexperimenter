#pragma once
#include "ioh/problem/utils.hpp"
#include "pbo_problem.hpp"

namespace ioh
{
    namespace problem
    {
        namespace pbo
        {
            class LeadingOnesRuggedness1 final: public PBOProblem<LeadingOnesRuggedness1>
            {

            protected:
                double evaluate(const std::vector<int> &x) override
                {
                    auto result = 0;
                    for (size_t i = 0; i != meta_data_.n_variables; ++i)
                        if (x[i] == 1)
                            result = static_cast<double>(i) + 1.;
                        else
                            break;
                    return utils::ruggedness1(result, meta_data_.n_variables);
                }

            public:
                /**
                 * \brief Construct a new LeadingOnes_Ruggedness1 object. Definition refers to
                 *https://doi.org/10.1016/j.asoc.2019.106027
                 *
                 * \param instance The instance number of a problem, which controls the transformation
                 * performed on the original problem.
                 * \param n_variables The dimensionality of the problem to created, 4 by default.
                 **/
                LeadingOnesRuggedness1(const int instance, const int n_variables) :
                    PBOProblem(15, instance, n_variables, "LeadingOnesRuggedness1")
                {
                    objective_.x = std::vector<int>(n_variables,1);
                    objective_.y = {n_variables / 2.0 + 1};
                }
            };
        } // namespace pbo
    } // namespace problem
} // namespace ioh
