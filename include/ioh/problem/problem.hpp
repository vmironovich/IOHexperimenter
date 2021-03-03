#pragma once

#include "utils.hpp"
#include "ioh/logger/base.hpp"

namespace ioh
{
    namespace problem
    {
     
        template <typename T>
        class Problem
        {
        protected:
            MetaData meta_data_; 
            Constraint<T> constraint_;
            State<T> state_;
            Solution<T> objective_;
            logger::Base *logger_{};

            [[nodiscard]]
            bool check_input_dimensions(const std::vector<T>& x)
            {
                if (x.empty())
                {
                    common::log::warning("The solution is empty.");
                    return false;
                }
                if (x.size() != meta_data_.n_variables)
                {
                    common::log::warning("The dimension of solution is incorrect.");
                    return false;
                }
                return true;
            }

            [[nodiscard]]
            virtual bool check_input(const std::vector<T>& x)
            {
                return check_input_dimensions(x);
            }

            [[nodiscard]]
            virtual std::vector<double> evaluate(const std::vector<T> &x) = 0;


            [[nodiscard]]
            virtual std::vector<T> transform_variables(std::vector<T> x)
            {
                return x;
            }

            [[nodiscard]]
            virtual std::vector<double> transform_objectives(std::vector<double> y)
            {
                return y;
            }

        public:
            explicit Problem(MetaData meta_data, Constraint<T> constraint, Solution<T> objective) :
                meta_data_(std::move(meta_data)), constraint_(std::move(constraint)), objective_(std::move(objective))
            {
                state_ = {{
                    std::vector<T>(meta_data_.n_variables, std::numeric_limits<T>::signaling_NaN()),
                    std::vector<double>(meta_data_.n_objectives, meta_data_.initial_objective_value)
                }};
                constraint_.check_size(meta_data_.n_variables);
            }

            explicit Problem(MetaData meta_data, Constraint<T> constraint = Constraint<T>()):
                Problem(meta_data, constraint, {
                            std::vector<T>(meta_data.n_variables, std::numeric_limits<T>::signaling_NaN()),
                            std::vector<double>(meta_data.n_objectives,
                                                (meta_data.optimization_type == common::OptimizationType::minimization
                                                    ? -std::numeric_limits<double>::infinity()
                                                    : std::numeric_limits<double>::infinity()))})
            {
            }

            virtual ~Problem() = default;

            virtual void reset()
            {
                state_.reset();
                if (logger_ != nullptr)
                    logger_->track_problem(meta_data_);
            }

            [[nodiscard]]
            virtual logger::LogInfo log_info() const
            {
                return {
                    static_cast<size_t>(state_.evaluations),
                    state_.current_best_internal.y.at(0),
                    state_.current.y.at(0),
                    state_.current_best.y.at(0),
                    {std::vector<double>(state_.current.x.begin(), state_.current.x.end()), state_.current.y},
                    {std::vector<double>(objective_.x.begin(), objective_.x.end()), objective_.y}
                };
            }

            void attach_logger(logger::Base &logger)
            {
                logger_ = &logger;
                logger_->track_problem(meta_data_);
            }

            void detach_logger()
            {
                if (logger_ != nullptr)
                    logger_->flush();
                logger_ = nullptr;
            }

            std::vector<double> operator()(const std::vector<T> &x)
            {
                if (!check_input(x)) 
                    return std::vector<double>(meta_data_.n_objectives, std::numeric_limits<T>::signaling_NaN());

                state_.current.x = x;
                state_.current_internal.x = transform_variables(x);
                state_.current_internal.y = evaluate(state_.current_internal.x);
                state_.current.y = transform_objectives(state_.current_internal.y);
                state_.update(meta_data_, objective_);
                if (logger_ != nullptr)
                    logger_->log(log_info());
                return state_.current.y;
            }

            [[nodiscard]]
            MetaData meta_data() const
            {
                return meta_data_;
            }

            [[nodiscard]]
            Solution<T> objective() const
            {
                return objective_;
            }

            [[nodiscard]]
            State<T> state() const
            {
                return state_;
            }

            [[nodiscard]]
            Constraint<T> constraint() const
            {
                return constraint_;
            }

            friend std::ostream &operator<<(std::ostream &os, const Problem &obj)
            {
                return os
                    << "Problem(\n\t" << obj.meta_data_
                    << "\n\tconstraint: " << obj.constraint_
                    << "\n\tstate: " << obj.state_ << "\n)";
            }
        };


        template <typename T>
        using Function = std::function<std::vector<double>(std::vector<T> &)>;

        template <typename T>
        class WrappedProblem final : public Problem<T>
            //TODO: make this class registerable
        {
        protected:
            Function<T> function_;

            std::vector<double> evaluate(const std::vector<T> &x) override
            {
                return function_(x);
            }

        public:
            WrappedProblem(Function<T> f, const std::string &name, const int n_variables, const int n_objectives = 1,
                           const common::OptimizationType optimization_type = common::OptimizationType::minimization,
                           Constraint<T> constraint = Constraint<T>()
                ) :
                Problem<T>(MetaData(0, name, n_variables, n_objectives, optimization_type), constraint),
                function_(f)
            {
            }
        };

        template <typename T>
        WrappedProblem<T> wrap_function(Function<T> f, const std::string &name, const int n_variables,
                                        const int n_objectives = 1,
                                        const common::OptimizationType optimization_type =
                                            common::OptimizationType::minimization,
                                        Constraint<T> constraint = Constraint<T>())
        {
            return WrappedProblem<T>{f, name, n_variables, n_objectives, optimization_type, constraint};
        }

        template <typename ProblemType>
        using ProblemFactoryType = common::RegisterWithFactory<ProblemType, int, int>;

        template <class Derived, class Parent>
        using AutomaticProblemRegistration = common::AutomaticTypeRegistration<Derived, ProblemFactoryType<Parent>>;

        template <class Parent>
        using ProblemRegistry = ProblemFactoryType<Parent>;
    
        
        using Real = Problem<double>;
        using Integer = Problem<int>;

        template<typename ProblemType>
        class RealProblem : public Real, AutomaticProblemRegistration<ProblemType, Real>
        {
        protected:
            bool check_input(const std::vector<double>& x) override
            {
                if (!check_input_dimensions(x))
                    return false;
                
                if (common::all_finite(x))
                    return true;
                
                if (common::has_nan(x))
                {
                    common::log::warning("The solution contains NaN.");
                    return false;
                }
                if (common::has_inf(x))
                {
                    common::log::warning("The solution contains Inf.");
                    return false;
                }
                common::log::warning("The solution contains invalid values.");
                return false;
            }
        public:
            using Real::Real;
        };

        template<typename ProblemType>
        struct IntegerProblem : Integer, AutomaticProblemRegistration<ProblemType, Integer>
        {
            using Integer::Integer;
        };
    }
}
