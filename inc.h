#include <bits/stdc++.h>
#include "BOBHash32.h"
using namespace std;
#define SZ(x) (int((x).size()))
#define All(x) (x).begin(), (x).end()
typedef pair<uint32_t, int> pii;
typedef pair<uint32_t, uint64_t> pil;
#define rep(i,a,b) for(int (i)=(a);(i)<=(b);++(i))
#define rep2(i,a,b) for(int (i)=(a);(i)<(b);++(i))
#define fi first
#define se second
bool cmp(const pii & A, const pii & B) {
    return A.se > B.se;
}

long long rd() {
    return 1ll*rand()*rand()+rand();
}

uint32_t getPrefix(uint32_t ip, int n) {
    if (n == 0) return 0;
    if (n == 32) return ip;
    return ip >> (32 - n);
}
uint32_t convertIPv4ToUint32(char* ipAddress) {
    uint32_t result = 0;
    int octet =0;
    char ipCopy[40];
    strncpy(ipCopy,ipAddress,sizeof(ipCopy)-1);
    ipCopy[sizeof(ipCopy)-1]='\0';
    char* token=strtok(ipCopy,".");
    while(token !=nullptr) {
        octet =std::stoi(token);
        result=(result<<8)+octet;
        token = std::strtok(nullptr,".");
    }
    return result;
}
double getCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("Invalid input vectors");
    }

    double xMean = 0.0;
    double yMean = 0.0;

    for (size_t i = 0; i < x.size(); i++) {
        xMean += x[i];
        yMean += y[i];
    }

    xMean /= static_cast<double>(x.size());
    yMean /= static_cast<double>(y.size());

    double numerator = 0.0;
    double xSquare = 0.0;
    double ySquare = 0.0;

    for (size_t i = 0; i < x.size(); i++) {
        double dx = x[i] - xMean;
        double dy = y[i] - yMean;

        numerator += dx * dy;
        xSquare += dx * dx;
        ySquare += dy * dy;
    }

    double denominator = std::sqrt(xSquare * ySquare);

    if (denominator == 0.0) {
        throw std::runtime_error("Correlation coefficient is undefined");
    }

    return numerator / denominator;
}
double getQuantile(const vector<double>& data, double q) {
    if (data.empty()) {
        throw invalid_argument("Cannot compute quantile of empty data");
    }

    if (q <= 0.0) {
        return data.front();
    }

    if (q >= 1.0) {
        return data.back();
    }

    double pos = q * (data.size() - 1);

    size_t left = static_cast<size_t>(floor(pos));
    size_t right = static_cast<size_t>(ceil(pos));

    if (left == right) {
        return data[left];
    }

    double weight = pos - static_cast<double>(left);

    return data[left] * (1.0 - weight)
         + data[right] * weight;
}

vector<double> generateBModel(
    double b,
    uint64_t packetnum
) {
    if (b < 0.5 || b > 1.0) {
        throw invalid_argument("b must be in [0.5, 1.0]");
    }

    vector<uint64_t> current;
    current.push_back(packetnum);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> chooseSide(0, 1);

    for (int p = 0; p < 32; p++) {
        vector<uint64_t> next;
        next.reserve(current.size() * 2);

        for (uint64_t parent : current) {
            if (parent == 0) {
                continue;
            }

            if (parent == 1) {
                next.push_back(1);
                continue;
            }

            uint64_t large =
                static_cast<uint64_t>(
                    llround(static_cast<double>(parent) * b)
                );

            if (large > parent) {
                large = parent;
            }

            uint64_t small = parent - large;

            if (chooseSide(gen) == 0) {
                if (large > 0) {
                    next.push_back(large);
                }

                if (small > 0) {
                    next.push_back(small);
                }
            } else {
                if (small > 0) {
                    next.push_back(small);
                }

                if (large > 0) {
                    next.push_back(large);
                }
            }
        }

        current.swap(next);
    }

    vector<double> modelData;
    modelData.reserve(current.size());

    uint64_t totalCheck = 0;

    for (uint64_t count : current) {
        if (count > 0) {
            modelData.push_back(
                static_cast<double>(count)
            );

            totalCheck += count;
        }
    }

    if (totalCheck != packetnum) {
        throw runtime_error(
            "Generated packet count does not match packetnum"
        );
    }

    return modelData;
}

double getCC( double b, const vector<map<uint32_t, int>>& mp) {
    if (mp.size() < 32) {
        throw invalid_argument(
            "mp must contain 32 prefix levels"
        );
    }

    vector<double> realData;

    uint64_t packetnum = 0;

    for (const auto& item : mp[31]) {
        if (item.second > 0) {
            realData.push_back(
                static_cast<double>(item.second)
            );

            packetnum +=
                static_cast<uint64_t>(item.second);
        }
    }

    if (realData.empty()) {
        throw invalid_argument(
            "No valid frequency data in mp[31]"
        );
    }

    vector<double> modelData =
        generateBModel(b, packetnum);

    sort(realData.begin(), realData.end());
    sort(modelData.begin(), modelData.end());

    vector<double> realQuantiles;
    vector<double> modelQuantiles;

    realQuantiles.reserve(100);
    modelQuantiles.reserve(100);

    for (int i = 1; i <= 100; i++) {
        double q = static_cast<double>(i) / 100.0;

        double realQ =
            getQuantile(realData, q);

        double modelQ =
            getQuantile(modelData, q);

        realQuantiles.push_back(realQ);
        modelQuantiles.push_back(modelQ);
    }

    return getCorrelation(
        realQuantiles,
        modelQuantiles
    );
}


using Histogram = std::map<uint64_t, uint64_t>;

Histogram getRealHistogram(const std::map<uint32_t, int>& ipFrequency, uint64_t& packetnum) {
    Histogram histogram;
    packetnum = 0;

    for (const auto& item : ipFrequency) {
        if (item.second <= 0) {
            continue;
        }

        uint64_t frequency = static_cast<uint64_t>(item.second);
        histogram[frequency]++;
        packetnum += frequency;
    }

    return histogram;
}

Histogram generateBModelHistogram(double b, uint64_t packetnum) {
    if (b < 0.5 || b > 1.0) {
        throw std::invalid_argument("b must be in [0.5, 1.0]");
    }

    Histogram current;
    current[packetnum] = 1;

    for (int p = 0; p < 32; p++) {
        Histogram next;

        for (const auto& item : current) {
            uint64_t parent = item.first;
            uint64_t multiplicity = item.second;

            if (parent == 1) {
                next[1] += multiplicity;
                continue;
            }

            uint64_t large = static_cast<uint64_t>(std::llround(static_cast<double>(parent) * b));

            if (large > parent) {
                large = parent;
            }

            uint64_t small = parent - large;

            if (large > 0) {
                next[large] += multiplicity;
            }

            if (small > 0) {
                next[small] += multiplicity;
            }
        }

        current.swap(next);
    }

    return current;
}

double getValueAtRank(const Histogram& histogram, uint64_t rank) {
    uint64_t current = 0;

    for (const auto& item : histogram) {
        if (rank < current + item.second) {
            return static_cast<double>(item.first);
        }

        current += item.second;
    }

    return static_cast<double>(histogram.rbegin()->first);
}
double getHistogramQuantile(const Histogram& histogram, double q) {
    if (histogram.empty()) {
        throw std::invalid_argument("Histogram is empty");
    }

    uint64_t size = 0;

    for (const auto& item : histogram) {
        size += item.second;
    }

    if (size == 1) {
        return static_cast<double>(histogram.begin()->first);
    }

    if (q >= 1.0) {
        return static_cast<double>(histogram.rbegin()->first);
    }

    double pos = q * static_cast<double>(size - 1);
    uint64_t left = static_cast<uint64_t>(std::floor(pos));
    uint64_t right = static_cast<uint64_t>(std::ceil(pos));

    double leftValue = getValueAtRank(histogram, left);

    if (left == right) {
        return leftValue;
    }

    double rightValue = getValueAtRank(histogram, right);
    double weight = pos - static_cast<double>(left);

    return leftValue * (1.0 - weight) + rightValue * weight;
}


double getCC1(double b, const std::vector<std::map<uint32_t, int>>& mp, int num) {
    if (mp.size() < 32) {
        throw std::invalid_argument("mp must contain 32 prefix levels");
    }

    if (num < 2) {
        throw std::invalid_argument("num must be at least 2");
    }

    uint64_t packetnum = 0;

    Histogram realHistogram = getRealHistogram(mp[31], packetnum);
    Histogram modelHistogram = generateBModelHistogram(b, packetnum);

    std::vector<double> realQuantiles;
    std::vector<double> modelQuantiles;

    realQuantiles.reserve(num);
    modelQuantiles.reserve(num);

    for (int i = 1; i <= num; i++) {
        double q = static_cast<double>(i) / static_cast<double>(num);

        realQuantiles.push_back(getHistogramQuantile(realHistogram, q));
        modelQuantiles.push_back(getHistogramQuantile(modelHistogram, q));
    }

    return getCorrelation(realQuantiles, modelQuantiles);
}


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