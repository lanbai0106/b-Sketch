// #include <vector>
// #include <cmath>
// #include <cstdint>
// #include <stdexcept>
// #include <random>
// #include <limits>
// #include <iostream>
// #include <algorithm>
//
//
// template<uint32_t key_len>
// struct Separate {
//
//     static constexpr int PURE_CM_LEVELS = 15;
//
//     struct HHCell {
//         uint32_t key;
//         int v;
//
//         HHCell() : key(0), v(0) {}
//     };
//
//     struct HybridLevel {
//         vector<vector<HHCell>> hh;
//         vector<vector<uint32_t>> cm;
//
//         uint64_t cmPacketnum;
//
//         HybridLevel(
//             int c,
//             int l,
//             int d,
//             int m_small
//         )
//             : hh(
//                 c,
//                 vector<HHCell>(l)
//               ),
//               cm(
//                 d,
//                 vector<uint32_t>(m_small, 0)
//               ),
//               cmPacketnum(0) {}
//     };
//
//
//     int d;
//
//     int m;
//
//     int m_small;
//
//     int c;
//
//     int l;
//
//
//     BOBHash32 **hash;
//
//     BOBHash32 *hashx;
//
//
//     uint64_t packetnum;
//
//     vector<uint64_t> levelPacketnum;
//
//
//     // Standard CM for levels 1-15
//     vector<vector<vector<uint32_t>>> counter;
//
//
//     // HH + small CM for levels 16-32
//     vector<HybridLevel> hybrid;
//
//
//     Separate(
//         int d_,
//         int m_,
//         int m_small_,
//         int c_,
//         int l_
//     )
//         : d(d_),
//           m(m_),
//           m_small(m_small_),
//           c(c_),
//           l(l_),
//           packetnum(0) {
//
//         static_assert(
//             key_len >= 1 && key_len <= 32,
//             "key_len must be in [1, 32]"
//         );
//
//         if (
//             d <= 0 ||
//             m <= 0 ||
//             m_small <= 0 ||
//             c <= 0 ||
//             l <= 0
//         ) {
//             throw invalid_argument(
//                 "d, m, m_small, c and l must be positive"
//             );
//         }
//
//
//         levelPacketnum.resize(
//             key_len,
//             0
//         );
//
//
//         int pureLevels =
//             min<int>(
//                 PURE_CM_LEVELS,
//                 key_len
//             );
//
//
//         counter.resize(
//             pureLevels,
//             vector<vector<uint32_t>>(
//                 d,
//                 vector<uint32_t>(
//                     m,
//                     0
//                 )
//             )
//         );
//
//
//         if (key_len > PURE_CM_LEVELS) {
//
//             int hybridLevels =
//                 key_len - PURE_CM_LEVELS;
//
//             hybrid.reserve(
//                 hybridLevels
//             );
//
//             for (
//                 int i = 0;
//                 i < hybridLevels;
//                 i++
//             ) {
//
//                 hybrid.emplace_back(
//                     c,
//                     l,
//                     d,
//                     m_small
//                 );
//             }
//         }
//
//
//         hash =
//             new BOBHash32*[d];
//
//         random_device rd;
//
//
//         for (
//             int i = 0;
//             i < d;
//             i++
//         ) {
//
//             hash[i] =
//                 new BOBHash32(
//                     uint8_t(
//                         rd() % MAX_PRIME32
//                     )
//                 );
//         }
//
//
//         hashx =
//             new BOBHash32(
//                 uint8_t(
//                     rd() % MAX_PRIME32
//                 )
//             );
//     }
//
//
//     ~Separate() {
//
//         for (
//             int i = 0;
//             i < d;
//             i++
//         ) {
//             delete hash[i];
//         }
//
//         delete[] hash;
//
//         delete hashx;
//     }
//
//
//     Separate(
//         const Separate&
//     ) = delete;
//
//
//     Separate&
//     operator=(
//         const Separate&
//     ) = delete;
//
//
//
//     void insertPureCM(
//         int level,
//         uint32_t key
//     ) {
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             uint32_t hashid =
//                 hash[row]->run(
//                     (char *)&key,
//                     4
//                 ) % m;
//
//
//             counter[
//                 level
//             ][
//                 row
//             ][
//                 hashid
//             ]++;
//         }
//     }
//
//
//
//     void insertSmallCM(
//         HybridLevel& module,
//         uint32_t key
//     ) {
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             uint32_t hashid =
//                 hash[row]->run(
//                     (char *)&key,
//                     4
//                 ) % m_small;
//
//
//             module.cm[
//                 row
//             ][
//                 hashid
//             ]++;
//         }
//
//
//         module.cmPacketnum++;
//     }
//
//
//
//     void insertHybrid(
//         HybridLevel& module,
//         uint32_t key
//     ) {
//
//         uint32_t bucketID =
//             hashx->run(
//                 (char *)&key,
//                 4
//             ) % c;
//
//
//         auto& bucket =
//             module.hh[bucketID];
//
//
//         // Check whether the key already exists
//         for (
//             auto& cell : bucket
//         ) {
//
//             if (
//                 cell.v > 0 &&
//                 cell.key == key
//             ) {
//
//                 cell.v++;
//
//                 return;
//             }
//         }
//
//
//         // Find an empty cell
//         for (
//             auto& cell : bucket
//         ) {
//
//             if (
//                 cell.v == 0
//             ) {
//
//                 cell.key = key;
//
//                 cell.v = 1;
//
//                 return;
//             }
//         }
//
//
//         // Find the cell with the minimum counter
//         int minIndex = 0;
//
//
//         for (
//             int i = 1;
//             i < l;
//             i++
//         ) {
//
//             if (
//                 bucket[i].v
//                 <
//                 bucket[minIndex].v
//             ) {
//
//                 minIndex = i;
//             }
//         }
//
//
//         HHCell& minCell =
//             bucket[minIndex];
//
//
//         minCell.v--;
//
//
//         // Replace the old key if the counter becomes zero
//         if (
//             minCell.v == 0
//         ) {
//
//             minCell.key = key;
//
//             minCell.v = 1;
//
//             return;
//         }
//
//
//         // Otherwise insert the current key into the small CM
//         insertSmallCM(
//             module,
//             key
//         );
//     }
//
//
//
//     void insert(
//         const vector<uint32_t>& key_list,
//         double p
//     ) {
//
//         int levels =
//             static_cast<int>(p);
//
//
//         if (
//             levels <= 0 ||
//             levels >
//             static_cast<int>(key_len)
//         ) {
//
//             throw invalid_argument(
//                 "invalid number of levels"
//             );
//         }
//
//
//         if (
//             key_list.size()
//             <
//             static_cast<size_t>(
//                 levels
//             )
//         ) {
//
//             throw invalid_argument(
//                 "key_list does not contain enough prefix keys"
//             );
//         }
//
//
//         packetnum++;
//
//
//         for (
//             int level = 0;
//             level < levels;
//             level++
//         ) {
//
//             uint32_t key =
//                 key_list[level];
//
//
//             if (
//                 level < PURE_CM_LEVELS
//             ) {
//
//                 insertPureCM(
//                     level,
//                     key
//                 );
//             }
//             else {
//
//                 int hybridID =
//                     level
//                     -
//                     PURE_CM_LEVELS;
//
//
//                 insertHybrid(
//                     hybrid[hybridID],
//                     key
//                 );
//             }
//
//
//             levelPacketnum[level]++;
//         }
//     }
//
//
//
//     double estimateF2(
//         int sketchID
//     ) const {
//
//         double minF2 =
//             numeric_limits<double>::infinity();
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             double rowF2 =
//                 0.0;
//
//
//             for (
//                 int col = 0;
//                 col < m;
//                 col++
//             ) {
//
//                 double cnt =
//                     static_cast<double>(
//                         counter[
//                             sketchID
//                         ][
//                             row
//                         ][
//                             col
//                         ]
//                     );
//
//
//                 rowF2 +=
//                     cnt * cnt;
//             }
//
//
//             if (
//                 rowF2 < minF2
//             ) {
//
//                 minF2 =
//                     rowF2;
//             }
//         }
//
//
//         return minF2;
//     }
//
//
//
//     double estimateEntropy(
//         int sketchID,
//         double total
//     ) const {
//
//         double minRawEntropy =
//             numeric_limits<double>::infinity();
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             double rawEntropy =
//                 0.0;
//
//
//             for (
//                 int col = 0;
//                 col < m;
//                 col++
//             ) {
//
//                 double cnt =
//                     static_cast<double>(
//                         counter[
//                             sketchID
//                         ][
//                             row
//                         ][
//                             col
//                         ]
//                     );
//
//
//                 if (
//                     cnt > 0.0
//                 ) {
//
//                     rawEntropy +=
//                         cnt *
//                         log2(cnt);
//                 }
//             }
//
//
//             if (
//                 rawEntropy
//                 <
//                 minRawEntropy
//             ) {
//
//                 minRawEntropy =
//                     rawEntropy;
//             }
//         }
//
//
//         return
//             log2(total)
//             -
//             minRawEntropy
//             /
//             total;
//     }
//
//
//
//     double estimateEntropy_(
//         int sketchID,
//         double total
//     ) const {
//
//         double maxEntropy =
//             0.0;
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             double entropy =
//                 0.0;
//
//
//             for (
//                 int col = 0;
//                 col < m;
//                 col++
//             ) {
//
//                 double cnt =
//                     static_cast<double>(
//                         counter[
//                             sketchID
//                         ][
//                             row
//                         ][
//                             col
//                         ]
//                     );
//
//
//                 if (
//                     cnt > 0.0
//                 ) {
//
//                     double prob =
//                         cnt /
//                         total;
//
//
//                     entropy -=
//                         prob *
//                         log2(prob);
//                 }
//             }
//
//
//             if (
//                 entropy
//                 >
//                 maxEntropy
//             ) {
//
//                 maxEntropy =
//                     entropy;
//             }
//         }
//
//
//         return maxEntropy;
//     }
//
//
//
//     uint32_t querySmallCM(
//         const HybridLevel& module,
//         uint32_t key
//     ) const {
//
//         uint32_t result =
//             numeric_limits<uint32_t>::max();
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             uint32_t hashid =
//                 hash[row]->run(
//                     (char *)&key,
//                     4
//                 ) % m_small;
//
//
//             result =
//                 min(
//                     result,
//                     module.cm[
//                         row
//                     ][
//                         hashid
//                     ]
//                 );
//         }
//
//
//         return result;
//     }
//
//
//
//     uint32_t queryHybridFrequency(
//         int hybridID,
//         uint32_t key
//     ) const {
//
//         const HybridLevel& module =
//             hybrid[hybridID];
//
//
//         uint32_t bucketID =
//             hashx->run(
//                 (char *)&key,
//                 4
//             ) % c;
//
//
//         const auto& bucket =
//             module.hh[bucketID];
//
//
//         for (
//             const auto& cell : bucket
//         ) {
//
//             if (
//                 cell.v > 0 &&
//                 cell.key == key
//             ) {
//
//                 return
//                     static_cast<uint32_t>(
//                         cell.v
//                     );
//             }
//         }
//
//
//         return querySmallCM(
//             module,
//             key
//         );
//     }
//
//
//
//     double estimateHHF2(
//         const HybridLevel& module
//     ) const {
//
//         double F2 =
//             0.0;
//
//
//         for (
//             const auto& bucket :
//             module.hh
//         ) {
//
//             for (
//                 const auto& cell :
//                 bucket
//             ) {
//
//                 if (
//                     cell.v > 0
//                 ) {
//
//                     double cnt =
//                         static_cast<double>(
//                             cell.v
//                         );
//
//
//                     F2 +=
//                         cnt * cnt;
//                 }
//             }
//         }
//
//
//         return F2;
//     }
//
//
//
//     double estimateHHRawEntropy(
//         const HybridLevel& module
//     ) const {
//
//         double rawEntropy =
//             0.0;
//
//
//         for (
//             const auto& bucket :
//             module.hh
//         ) {
//
//             for (
//                 const auto& cell :
//                 bucket
//             ) {
//
//                 if (
//                     cell.v > 0
//                 ) {
//
//                     double cnt =
//                         static_cast<double>(
//                             cell.v
//                         );
//
//
//                     rawEntropy +=
//                         cnt *
//                         log2(cnt);
//                 }
//             }
//         }
//
//
//         return rawEntropy;
//     }
//
//
//
//     double estimateSmallCMF2(
//         const HybridLevel& module
//     ) const {
//
//         if (
//             module.cmPacketnum == 0
//         ) {
//             return 0.0;
//         }
//
//
//         double minF2 =
//             numeric_limits<double>::infinity();
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             double rowF2 =
//                 0.0;
//
//
//             for (
//                 int col = 0;
//                 col < m_small;
//                 col++
//             ) {
//
//                 double cnt =
//                     static_cast<double>(
//                         module.cm[
//                             row
//                         ][
//                             col
//                         ]
//                     );
//
//
//                 rowF2 +=
//                     cnt * cnt;
//             }
//
//
//             if (
//                 rowF2 < minF2
//             ) {
//
//                 minF2 =
//                     rowF2;
//             }
//         }
//
//
//         return minF2;
//     }
//
//
//
//     double estimateSmallCMRawEntropy(
//         const HybridLevel& module
//     ) const {
//
//         if (
//             module.cmPacketnum == 0
//         ) {
//             return 0.0;
//         }
//
//
//         double minRawEntropy =
//             numeric_limits<double>::infinity();
//
//
//         for (
//             int row = 0;
//             row < d;
//             row++
//         ) {
//
//             double rawEntropy =
//                 0.0;
//
//
//             for (
//                 int col = 0;
//                 col < m_small;
//                 col++
//             ) {
//
//                 double cnt =
//                     static_cast<double>(
//                         module.cm[
//                             row
//                         ][
//                             col
//                         ]
//                     );
//
//
//                 if (
//                     cnt > 0.0
//                 ) {
//
//                     rawEntropy +=
//                         cnt *
//                         log2(cnt);
//                 }
//             }
//
//
//             if (
//                 rawEntropy
//                 <
//                 minRawEntropy
//             ) {
//
//                 minRawEntropy =
//                     rawEntropy;
//             }
//         }
//
//
//         return minRawEntropy;
//     }
//
//
//
//     double estimateHybridF2(
//         int hybridID
//     ) const {
//
//         const HybridLevel& module =
//             hybrid[hybridID];
//
//
//         double hhF2 =
//             estimateHHF2(
//                 module
//             );
//
//
//         double cmF2 =
//             estimateSmallCMF2(
//                 module
//             );
//
//
//         return
//             hhF2
//             +
//             cmF2;
//     }
//
//
//
//     double estimateHybridEntropy(
//         int hybridID,
//         double total
//     ) const {
//
//         const HybridLevel& module =
//             hybrid[hybridID];
//
//
//         double hhRawEntropy =
//             estimateHHRawEntropy(
//                 module
//             );
//
//
//         double cmRawEntropy =
//             estimateSmallCMRawEntropy(
//                 module
//             );
//
//
//         double rawEntropy =
//             hhRawEntropy
//             +
//             cmRawEntropy;
//
//
//         return
//             log2(total)
//             -
//             rawEntropy
//             /
//             total;
//     }
//
//
//
//     double correctF2(
//         double estimatedF2,
//         double total,
//         double width
//     ) const {
//
//         if (
//             width <= 1.0
//         ) {
//
//             return estimatedF2;
//         }
//
//
//         return
//             (
//                 width *
//                 estimatedF2
//                 -
//                 total *
//                 total
//             )
//             /
//             (
//                 width
//                 -
//                 1.0
//             );
//     }
//
//
//
//     double estimateHybridCorrectedF2(
//         int hybridID
//     ) const {
//
//         const HybridLevel& module =
//             hybrid[hybridID];
//
//
//         double hhF2 =
//             estimateHHF2(
//                 module
//             );
//
//
//         if (
//             module.cmPacketnum == 0
//         ) {
//
//             return hhF2;
//         }
//
//
//         double cmF2 =
//             estimateSmallCMF2(
//                 module
//             );
//
//
//         double cmTotal =
//             static_cast<double>(
//                 module.cmPacketnum
//             );
//
//
//         double correctedCMF2 =
//             correctF2(
//                 cmF2,
//                 cmTotal,
//                 static_cast<double>(
//                     m_small
//                 )
//             );
//
//
//         return
//             hhF2
//             +
//             correctedCMF2;
//     }
//
//
//
//     vector<double> query(
//         double p
//     ) const {
//
//         int levels =
//             static_cast<int>(p);
//
//
//         if (
//             packetnum == 0
//         ) {
//
//             throw runtime_error(
//                 "Separate is empty"
//             );
//         }
//
//
//         if (
//             levels <= 0 ||
//             levels >
//             static_cast<int>(
//                 key_len
//             )
//         ) {
//
//             throw invalid_argument(
//                 "invalid number of levels"
//             );
//         }
//
//
//         for (
//             int level = 0;
//             level < levels;
//             level++
//         ) {
//
//             if (
//                 levelPacketnum[level]
//                 !=
//                 packetnum
//             ) {
//
//                 throw runtime_error(
//                     "Not all packets were inserted into the queried levels"
//                 );
//             }
//         }
//
//
//         double total =
//             static_cast<double>(
//                 packetnum
//             );
//
//
//         vector<double> F2_x;
//         vector<double> F2_y;
//
//         vector<double> Var_x;
//         vector<double> Var_y;
//
//         vector<double> Ent_x;
//         vector<double> Ent_y;
//
//
//         F2_x.reserve(
//             levels + 1
//         );
//
//         F2_y.reserve(
//             levels + 1
//         );
//
//         Var_x.reserve(
//             levels
//         );
//
//         Var_y.reserve(
//             levels
//         );
//
//         Ent_x.reserve(
//             levels + 1
//         );
//
//         Ent_y.reserve(
//             levels + 1
//         );
//
//
//         double F2_0 =
//             total *
//             total;
//
//
//         F2_x.push_back(
//             0.0
//         );
//
//         F2_y.push_back(
//             log2(F2_0)
//         );
//
//
//         Ent_x.push_back(
//             0.0
//         );
//
//         Ent_y.push_back(
//             0.0
//         );
//
//
//         for (
//             int level = 1;
//             level <= levels;
//             level++
//         ) {
//
//             double F2 =
//                 0.0;
//
//             double correctedF2 =
//                 0.0;
//
//             double entropy =
//                 0.0;
//
//
//             if (
//                 level <=
//                 PURE_CM_LEVELS
//             ) {
//
//                 int sketchID =
//                     level - 1;
//
//
//                 F2 =
//                     estimateF2(
//                         sketchID
//                     );
//
//
//                 correctedF2 =
//                     correctF2(
//                         F2,
//                         total,
//                         static_cast<double>(
//                             m
//                         )
//                     );
//
//
//                 entropy =
//                     estimateEntropy(
//                         sketchID,
//                         total
//                     );
//             }
//             else {
//
//                 int hybridID =
//                     level
//                     -
//                     PURE_CM_LEVELS
//                     -
//                     1;
//
//
//                 F2 =
//                     estimateHybridF2(
//                         hybridID
//                     );
//
//
//                 correctedF2 =
//                     estimateHybridCorrectedF2(
//                         hybridID
//                     );
//
//
//                 entropy =
//                     estimateHybridEntropy(
//                         hybridID,
//                         total
//                     );
//             }
//
//
//             F2_x.push_back(
//                 static_cast<double>(
//                     level
//                 )
//             );
//
//
//             F2_y.push_back(
//                 log2(
//                     correctedF2
//                 )
//             );
//
//
//             Ent_x.push_back(
//                 static_cast<double>(
//                     level
//                 )
//             );
//
//
//             Ent_y.push_back(
//                 entropy
//             );
//
//
//             double nodeNum =
//                 exp2(
//                     static_cast<double>(
//                         level
//                     )
//                 );
//
//
//             double variance =
//                 nodeNum
//                 *
//                 F2
//                 /
//                 (
//                     total *
//                     total
//                 )
//                 -
//                 1.0;
//
//
//             if (
//                 variance > 0.0
//             ) {
//
//                 Var_x.push_back(
//                     -static_cast<double>(
//                         level
//                     )
//                 );
//
//
//                 Var_y.push_back(
//                     log2(
//                         variance
//                     )
//                 );
//             }
//
//
//             cout
//                 << "p = "
//                 << level
//
//                 << " Fractal = "
//                 << log2(F2)
//
//                 << " CorrectedFractal = "
//                 << log2(correctedF2)
//
//                 << " Variance = ";
//
//
//             if (
//                 variance > 0.0
//             ) {
//
//                 cout
//                     << log2(
//                         variance
//                     );
//             }
//             else {
//
//                 cout
//                     << "-inf";
//             }
//
//
//             cout
//                 << " Entropy = "
//                 << entropy
//                 << endl;
//         }
//
//
//         if (
//             F2_x.size() < 2 ||
//             Ent_x.size() < 2 ||
//             Var_x.size() < 2
//         ) {
//
//             throw runtime_error(
//                 "Not enough valid levels for regression"
//             );
//         }
//
//
//         double F2Slope =
//             getRatio(
//                 F2_x,
//                 F2_y
//             );
//
//
//         double varSlope =
//             getRatio(
//                 Var_x,
//                 Var_y
//             );
//
//
//         double entSlope =
//             getRatio(
//                 Ent_x,
//                 Ent_y
//             );
//
//
//         double bF2 =
//             getBFromFractal(
//                 F2Slope
//             );
//
//
//         double bVariance =
//             getBFromVariance(
//                 varSlope
//             );
//
//
//         double bEntropy =
//             getBFromEntropy(
//                 entSlope
//             );
//
//
//         return {
//             bF2,
//             bVariance,
//             bEntropy
//         };
//     }
// };

#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <random>
#include <limits>
#include <iostream>
#include <algorithm>

template<uint32_t key_len>
struct Separate {
    static constexpr int PURE_CM_LEVELS = 12;

    struct HHCell {
        uint32_t key = 0;
        int v = 0;
    };

    struct HybridLevel {
        std::vector<std::vector<HHCell>> hh;
        std::vector<std::vector<uint32_t>> cm;
        uint64_t cmPacketnum = 0;

        HybridLevel(int c, int l, int d, int m_small)
            : hh(c, std::vector<HHCell>(l)),
              cm(d, std::vector<uint32_t>(m_small, 0)) {}
    };

    struct Stats {
        double F2;
        double correctedF2;
        double entropy;
    };

    int d;
    int m;
    int m_small;
    int c;
    int l;

    BOBHash32 **hash;
    BOBHash32 *hashx;

    uint64_t packetnum = 0;

    std::vector<std::vector<std::vector<uint32_t>>> counter;
    std::vector<HybridLevel> hybrid;

    Separate(int d_, int m_, int m_small_, int c_, int l_)
        : d(d_), m(m_), m_small(m_small_), c(c_), l(l_) {

        static_assert(key_len >= 1 && key_len <= 32,
                      "key_len must be in [1, 32]");

        if (d <= 0 || m <= 1 || m_small <= 1 || c <= 0 || l <= 0) {
            throw std::invalid_argument("invalid parameters");
        }

        int pureLevels = std::min<int>(PURE_CM_LEVELS, key_len);

        counter.resize(
            pureLevels,
            std::vector<std::vector<uint32_t>>(
                d,
                std::vector<uint32_t>(m, 0)
            )
        );

        for (int i = PURE_CM_LEVELS; i < key_len; i++) {
            hybrid.emplace_back(c, l, d, m_small);
        }

        std::random_device rd;

        hash = new BOBHash32*[d];

        for (int i = 0; i < d; i++) {
            hash[i] =
                new BOBHash32(uint8_t(rd() % MAX_PRIME32));
        }

        hashx =
            new BOBHash32(uint8_t(rd() % MAX_PRIME32));
    }

    ~Separate() {
        for (int i = 0; i < d; i++) {
            delete hash[i];
        }

        delete[] hash;
        delete hashx;
    }

    Separate(const Separate&) = delete;
    Separate& operator=(const Separate&) = delete;


    void insertHybrid(HybridLevel& module, uint32_t key) {
        uint32_t bucketID =
            hashx->run((char*)&key, 4) % c;

        auto& bucket = module.hh[bucketID];

        HHCell* emptyCell = nullptr;
        HHCell* minCell = &bucket[0];

        for (auto& cell : bucket) {
            if (cell.v > 0 && cell.key == key) {
                cell.v++;
                return;
            }

            if (cell.v == 0 && emptyCell == nullptr) {
                emptyCell = &cell;
            }

            if (cell.v < minCell->v) {
                minCell = &cell;
            }
        }

        if (emptyCell != nullptr) {
            emptyCell->key = key;
            emptyCell->v = 1;
            return;
        }

        minCell->v--;

        if (minCell->v == 0) {
            minCell->key = key;
            minCell->v = 1;
            return;
        }
        // double replaceProb = 1.0 / (static_cast<double>(minCell->v));
        // double randomValue = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        //
        // if (randomValue < replaceProb) {
        //     minCell->key = key;
        //     minCell->v++;
        //     return;
        // }
        //
        for (int row = 0; row < d; row++) {
            uint32_t hashid = hash[row]->run((char*)&key, 4) % m_small;
            module.cm[row][hashid]++;
        }

        module.cmPacketnum++;
    }


    void insert(const std::vector<uint32_t>& key_list, int levels) {
        if (levels <= 0 ||
            levels > static_cast<int>(key_len) ||
            key_list.size() < static_cast<size_t>(levels)) {
            throw std::invalid_argument("invalid levels");
        }

        packetnum++;

        for (int level = 0; level < levels; level++) {
            uint32_t key = key_list[level];

            if (level < PURE_CM_LEVELS) {
                for (int row = 0; row < d; row++) {
                    uint32_t hashid =
                        hash[row]->run((char*)&key, 4) % m;

                    counter[level][row][hashid]++;
                }
            }
            else {
                insertHybrid(
                    hybrid[level - PURE_CM_LEVELS],
                    key
                );
            }
        }
    }


    Stats estimateF2Entropy(int sketchID, double total) const {
        double minF2 =
            std::numeric_limits<double>::infinity();

        double minRawEntropy =
            std::numeric_limits<double>::infinity();

        for (int row = 0; row < d; row++) {
            double rowF2 = 0.0;
            double rawEntropy = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt =
                    static_cast<double>(
                        counter[sketchID][row][col]
                    );

                rowF2 += cnt * cnt;

                if (cnt > 0.0) {
                    rawEntropy += cnt * std::log2(cnt);
                }
            }

            if (rowF2 < minF2) {
                minF2 = rowF2;
            }

            if (rawEntropy < minRawEntropy) {
                minRawEntropy = rawEntropy;
            }
        }

        double correctedF2 =
            (m * minF2 - total * total) /
            (m - 1.0);

        double entropy =
            std::log2(total) -
            minRawEntropy / total;

        return {
            minF2,
            correctedF2,
            entropy
        };
    }


    Stats estimateHybrid(int hybridID, double total) const {
        const auto& module = hybrid[hybridID];

        double hhF2 = 0.0;
        double hhRawEntropy = 0.0;

        for (const auto& bucket : module.hh) {
            for (const auto& cell : bucket) {
                if (cell.v > 0) {
                    double cnt =
                        static_cast<double>(cell.v);

                    hhF2 += cnt * cnt;
                    hhRawEntropy +=
                        cnt * std::log2(cnt);
                }
            }
        }

        double cmF2 = 0.0;
        double cmRawEntropy = 0.0;

        if (module.cmPacketnum > 0) {
            cmF2 =
                std::numeric_limits<double>::infinity();

            cmRawEntropy =
                std::numeric_limits<double>::infinity();

            for (int row = 0; row < d; row++) {
                double rowF2 = 0.0;
                double rawEntropy = 0.0;

                for (int col = 0; col < m_small; col++) {
                    double cnt =
                        static_cast<double>(
                            module.cm[row][col]
                        );

                    rowF2 += cnt * cnt;

                    if (cnt > 0.0) {
                        rawEntropy +=
                            cnt * std::log2(cnt);
                    }
                }

                if (rowF2 < cmF2) {
                    cmF2 = rowF2;
                }

                if (rawEntropy < cmRawEntropy) {
                    cmRawEntropy = rawEntropy;
                }
            }
        }

        double correctedCMF2 = cmF2;


        if (module.cmPacketnum > 0) {
            double cmTotal =
                static_cast<double>(
                    module.cmPacketnum
                );

            correctedCMF2 =
                (
                    m_small * cmF2 -
                    cmTotal * cmTotal
                ) /
                (m_small - 1.0);
        }

        double F2 =
            hhF2 + cmF2;

        double correctedF2 =
            hhF2 + correctedCMF2;

        double entropy =
            std::log2(total) -
            (hhRawEntropy + cmRawEntropy) / total;

        return {
            F2,
            correctedF2,
            entropy
        };
    }


    std::vector<double> query(int levels) const {
        if (packetnum == 0) {
            throw std::runtime_error("Separate is empty");
        }

        double total =
            static_cast<double>(packetnum);

        std::vector<double> F2_x;
        std::vector<double> F2_y;

        std::vector<double> Var_x;
        std::vector<double> Var_y;

        std::vector<double> Ent_x;
        std::vector<double> Ent_y;

        F2_x.reserve(levels + 1);
        F2_y.reserve(levels + 1);

        Var_x.reserve(levels);
        Var_y.reserve(levels);

        Ent_x.reserve(levels + 1);
        Ent_y.reserve(levels + 1);

        F2_x.push_back(0.0);
        F2_y.push_back(std::log2(total * total));

        Ent_x.push_back(0.0);
        Ent_y.push_back(0.0);


        for (int level = 1; level <= levels; level++) {
            Stats stats;

            if (level <= PURE_CM_LEVELS) {
                stats =
                    estimateF2Entropy(
                        level - 1,
                        total
                    );
            }
            else {
                stats =
                    estimateHybrid(
                        level - PURE_CM_LEVELS - 1,
                        total
                    );
            }

            double F2 = stats.F2;
            double correctedF2 = stats.correctedF2;
            double entropy = stats.entropy;

            F2_x.push_back(
                static_cast<double>(level)
            );

            F2_y.push_back(
                std::log2(correctedF2)
            );

            Ent_x.push_back(
                static_cast<double>(level)
            );

            Ent_y.push_back(
                entropy
            );

            double nodeNum =
                std::exp2(
                    static_cast<double>(level)
                );

            double variance =
                nodeNum * F2 /
                (total * total) -
                1.0;

            if (variance > 0.0) {
                Var_x.push_back(
                    -static_cast<double>(level)
                );

                Var_y.push_back(
                    std::log2(variance)
                );
            }

            std::cout
                << "p = " << level
                << " Fractal = " << std::log2(F2)
                << " CorrectedFractal = "
                << std::log2(correctedF2)
                << " Variance = "
                << std::log2(variance)
                << " Entropy = "
                << entropy
                << std::endl;
        }


        double F2Slope =
            getRatio(F2_x, F2_y);

        double varSlope =
            getRatio(Var_x, Var_y);

        double entSlope =
            getRatio(Ent_x, Ent_y);


        double bF2 =
            getBFromFractal(F2Slope);

        double bVariance =
            getBFromVariance(varSlope);

        double bEntropy =
            getBFromEntropy(entSlope);


        return {
            bF2,
            bVariance,
            bEntropy
        };
    }
};