#include <fstream>
#include <queue>
#include <sstream>
#include "BOBHash32.h"
#include <filesystem>
#include <stdexcept>

vector<double> Frac_y, Var_y, Ent_y;
vector<double> Frac_x, Var_x, Ent_x;
vector<map<uint32_t, int> > mp;
class DataGenerator {
    private:
        vector<vector<uint32_t>> pac_list;
        string filePath;
        double fractal_slope, var_slope, ent_slope;
        vector<double> b_list;
    public:
        DataGenerator(string filePath,vector<vector<uint32_t>> pac_list): filePath(filePath), pac_list(pac_list) {
            if(filePath == "ca19") {
                filePath = "param/CAIDA19.txt";
            }else if(filePath == "ca16") {
                filePath = "param/CAIDA16.txt";
            }else if(filePath == "so") {
                filePath = "param/stackoverflow.txt";
            }else if(filePath == "wd") {
                filePath = "param/webdocs.txt";
            }
            for (int j = 0; j < 32; j++) {
                map<uint32_t, int> tmp;
                mp.push_back(tmp);
            }
            b_list = getb(filePath, pac_list);

        }
        vector<double> getb_list(){ return b_list;}
        double getRatio(const vector<double>& x, const vector<double>& y) {
            if (x.size() != y.size() || x.empty()) {
                throw invalid_argument(
                    "Input vectors must have the same non-zero size"
                );
            }

            size_t n = x.size();

            double x_mean = 0.0;
            double y_mean = 0.0;

            for (size_t i = 0; i < n; i++) {
                x_mean += x[i];
                y_mean += y[i];
            }

            x_mean /= static_cast<double>(n);
            y_mean /= static_cast<double>(n);

            double numerator = 0.0;
            double denominator = 0.0;

            for (size_t i = 0; i < n; i++) {
                double dx = x[i] - x_mean;
                double dy = y[i] - y_mean;

                numerator += dx * dy;
                denominator += dx * dx;
            }

            if (denominator == 0.0) {
                throw invalid_argument(
                    "Cannot compute slope when all x values are identical"
                );
            }

            return numerator / denominator;
        }

        double getBFromFractal(double slope) {
            double q = exp2(slope);

            double value = 2.0 * q - 1.0;

            if (value < 0.0) {
                throw invalid_argument("Invalid fractal slope");
            }

            return (1.0 + sqrt(value)) / 2.0;
        }

        double binaryEntropy(double b) {
            if (b <= 0.0 || b >= 1.0) {
                return 0.0;
            }

            return -b * log2(b)
                   - (1.0 - b) * log2(1.0 - b);
        }

        double getBFromEntropy(double slope) {
            if (slope < 0.0 || slope > 1.0) {
                throw invalid_argument("Invalid entropy slope");
            }

            double left = 0.5;
            double right = 1.0;

            for (int i = 0; i < 100; i++) {
                double mid = (left + right) / 2.0;
                double h = binaryEntropy(mid);

                if (h > slope) {
                    left = mid;
                } else {
                    right = mid;
                }
            }

            return (left + right) / 2.0;
        }

        double getBFromVariance(double slope) {
            double value = exp2(-slope) - 1.0;

            if (value < 0.0) {
                throw invalid_argument("Invalid variance slope");
            }

            return (1.0 + sqrt(value)) / 2.0;
        }

    void ge_data(const vector<vector<uint32_t>>& pac_list) {
        for (int i = 0; i < 32; i++) {
            mp[i].clear();
        }

        Frac_x.clear();
        Frac_y.clear();

        Var_x.clear();
        Var_y.clear();

        Ent_x.clear();
        Ent_y.clear();

        for (int i = 0; i < packetnum; i++) {
            const vector<uint32_t>& pac = pac_list[i];

            if (pac.size() < 32) {
                throw invalid_argument(
                    "Each packet must contain 32 prefix values"
                );
            }

            for (int j = 0; j < 32; j++) {
                mp[j][pac[j]]++;
            }
        }

        double total = static_cast<double>(packetnum);

        // p = 0

        double fr0 = total * total;

        Frac_x.push_back(0.0);
        Frac_y.push_back(log2(fr0));

        Ent_x.push_back(0.0);
        Ent_y.push_back(0.0);

        // p = 1 ... 32

        for (int i = 0; i < 32; i++) {
            int p = i + 1;

            const auto& current_level = mp[i];

            double node_num = static_cast<double>(1ULL << p);

            double fr = 0.0;
            double entropy = 0.0;

            for (const auto& item : current_level) {
                double count = static_cast<double>(item.second);

                fr += count * count;

                double probability = count / total;

                if (probability > 0.0) {
                    entropy -= probability * log2(probability);
                }
            }

            // Fractal
            // x = log2(2^p) = p
            // y = log2(N(p))

            Frac_x.push_back(static_cast<double>(p));
            Frac_y.push_back(log2(fr));

            // Entropy
            // x = p
            // y = E(p)

            Ent_x.push_back(static_cast<double>(p));
            Ent_y.push_back(entropy);

            // Variance
            // Y_k = (M_k / total) / 2^(-p)
            //
            // V(p)
            // = (1 / 2^p) * sum(Y_k^2) - 1
            // = 2^p * N(p) / total^2 - 1

            double variance =
                node_num * fr / (total * total) - 1.0;

            // p = 0 is not used because V(0) = 0.
            // Other zero values must also be excluded
            // because log2(0) is undefined.

            if (variance > 0.0) {
                Var_x.push_back(-static_cast<double>(p));
                Var_y.push_back(log2(variance));
            }

            cout
                << "p = " << p
                << " Fractal = " << log2(fr)
                << " Variance = " << log2(variance)
                << " Entropy = " << entropy
                << endl;
        }
    }

    vector<double> getb(const string& filePath,const vector<vector<uint32_t>>& pac_list) {
        (void)filePath;

        ge_data(pac_list);

        fractal_slope =
            getRatio(Frac_x, Frac_y);

        var_slope =
            getRatio(Var_x, Var_y);

        ent_slope =
            getRatio(Ent_x, Ent_y);

        double b_fractal =
            getBFromFractal(fractal_slope);

        double b_variance =
            getBFromVariance(var_slope);

        double b_entropy =
            getBFromEntropy(ent_slope);

        cout << endl;

        cout << "Fractal slope = " << fractal_slope << ", b = "
            << b_fractal  << endl;

        cout << "Variance slope = " << var_slope << ", b = " << b_variance << endl;

        cout << "Entropy slope = "  << ent_slope  << ", b = " << b_entropy  << endl;

        vector<double> b_list = {b_fractal, b_variance, b_entropy};
        return b_list;
    }
};
