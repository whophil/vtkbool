/*
Copyright 2012-2018 Ronald Römer

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <vector>
#include <iostream>
#include <memory>
#include <set>
#include <map>
#include <cassert>

#include "Tools.h"
#include "VisPoly.h"
#include "RmTrivials.h"

void GetVisPoly (PolyType &poly, PolyType &res, int ind) {
    double x[] = {poly[ind].x, poly[ind].y};

    VertsType verts;

    int num = poly.size();

    for (int i = 0; i < num-1; i++) {
        int _i = (ind+i+1)%num;
        double pt[] = {poly[_i].x, poly[_i].y};
        verts.push_back(Vert(x, pt, _i));
    }

    double ref[] = {verts[0].pt[0]-x[0], verts[0].pt[1]-x[1]};
    Normalize(ref);

    for (Vert& v : verts) {
        v.phi = GetAngle(ref, v.r);

        std::cout << v.phi*180/PI << std::endl;
    }

    for (int i = 0; i < num-2; i++) {
        verts[i].nxt = i+1;
    }

    for (Vert& v : verts) {
        std::cout << v.id << ", " << v.nxt << std::endl;
    }

    IdsType vp = {0, 1};

    int t = 0, u, v;

    std::vector<Bag> leftBags;

    for (;;) {
        u = verts[t].nxt;
        v = verts[u].nxt;

        if (v == NO_USE) {
            break;
        }

        std::cout << "> " << u << ", " << v << std::endl;

        double ptU[2], ptV[2];
        Cpy(ptU, verts[u].pt);
        Cpy(ptV, verts[v].pt);

        std::cout << "orig " << verts[u].id << ", " << verts[v].id << std::endl;

        if (Ld(x, ptU, ptV)) {
            std::cout << "skipping" << std::endl;

            t = u;
            continue;
        }

        double cA = Cross(x, ptU, ptV),
            cB = Cross(verts[t].pt, ptU, ptV);

        std::cout << "cA " << cA << std::endl;
        std::cout << "cB " << cB << std::endl;

        if (cA < 0) {
            std::cout << "vis" << std::endl;

            if (vp.back() != u) {
                vp.push_back(u);
            }

            vp.push_back(v);

            t = u;
        } else {
            if (cB > 0 || IsNear(verts[t].pt, ptV)) {

                int w = v;
                for (;;) {
                    Vert &a = verts[w],
                        &b = verts[a.nxt];

                    double *ptA = a.pt,
                        *ptB = b.pt;

                    std::cout << ">> " << a.id << ", " << b.id << std::endl;

                    std::shared_ptr<D> d(Intersect(x, verts[u].r, ptA, ptB));

                    if (d
                        && d->t1 > E
                        && IsFrontfaced(verts[u].r, ptA, ptB)) {

                        std::cout << "edge (" << a.id << ", " << b.id << ")" << std::endl;

                        if (d->t2 < E) {
                            verts[u].nxt = w;
                            vp.push_back(w);
                            t = u;
                            leftBags.push_back(Bag(u, w, verts[u].phi));

                        } else {

                            Vert _v(x, d->s, ref, a.nxt);
                            verts.push_back(_v);

                            int k = verts.size()-1;

                            verts[u].nxt = k;

                            vp.push_back(k);

                            t = u;

                            leftBags.push_back(Bag(u, k, verts[u].phi));

                        }

                        break;

                    } else {
                        w = a.nxt;
                    }

                }

            } else if (cB < 0) {
                // schnitt mit leftBags?

                std::shared_ptr<Bag> bag;
                std::shared_ptr<D> d;

                while (leftBags.size() > 0 && !d) {
                    bag = std::make_shared<Bag>(leftBags.back());

                    if (bag->phi > verts[v].phi || std::abs(bag->phi-verts[v].phi) < E) {
                        d = Intersect2(verts[bag->f].pt, verts[bag->g].pt, ptU, ptV);

                        leftBags.pop_back();
                    } else {
                        break;
                    }
                }

                if (d) {
                    std::cout << "bag " << *bag << std::endl;

                    while (vp.size() > 0 && vp.back() != bag->f) {
                        std::cout << "popping_1 " << vp.back() << std::endl;
                        vp.pop_back();
                    }

                    if (vp.size() < 2) {
                        throw vp_error();
                    }

                    int _x = v;

                    int i = 0;

                    for (;;) {
                        Vert &a = verts[_x],
                            &b = verts[a.nxt];

                        double *ptA = a.pt,
                            *ptB = b.pt;

                        std::cout << ">>" << a.id << ", " << b.id << std::endl;

                        std::shared_ptr<D> _d;

                        if (IsNear(verts[bag->f].pt, ptV)) {
                            _d = std::make_shared<D>(verts[bag->f].pt);
                        } else {
                            _d = Intersect2(verts[bag->f].pt, d->s, ptB, ptA);
                        }

                        if (_d
                            && IsFrontfaced(verts[bag->f].r, ptA, ptB)
                            && (i > 0 || Cross(ptA, ptU, ptB) < 0)) {

                            if (IsNear(verts[bag->f].pt, _d->s)) {
                                verts[bag->f].nxt = a.nxt;

                                vp.push_back(a.nxt);

                            } else {
                                if (_d->t2 > 1-E) {
                                    verts[bag->f].nxt = _x;

                                    vp.push_back(_x);

                                    leftBags.push_back(Bag(bag->f, _x, bag->phi));

                                } else {

                                    Vert _v(x, _d->s, ref, a.nxt);
                                    verts.push_back(_v);

                                    int k = verts.size()-1;

                                    verts[bag->f].nxt = k;

                                    vp.push_back(k);

                                    leftBags.push_back(Bag(bag->f, k, bag->phi));

                                }

                            }

                            t = bag->f;

                            break;

                        } else {
                            _x = a.nxt;
                        }

                        i++;


                    }

                } else {
                    while (vp.size() > 0) {
                        int a = vp.end()[-2],
                            b = vp.back();

                        std::cout << "popping_2 " << vp.back() << std::endl;

                        vp.pop_back();

                        if (vp.size() < 1) {
                            throw vp_error();
                        }

                        std::shared_ptr<D> d(Intersect(x, verts[v].r, verts[a].pt, verts[b].pt));

                        if (d) {
                            if (d->t2 < E) {
                                int c = vp.end()[-2];

                                if (Ld(x, verts[a].pt, verts[c].pt) || IsNear(verts[a].pt, ptV)) {
                                    vp.pop_back();
                                    t = vp.back();
                                } else {
                                    t = a;
                                }

                            } else {
                                Vert _v(x, d->s, ref);
                                verts.push_back(_v);

                                int k = verts.size()-1;

                                verts[a].nxt = k;

                                vp.push_back(k);

                                t = k;

                            }

                            break;

                        }

                    }

                    int p = v;

                    int w = verts[v].nxt;

                    if (Ld(x, ptV, verts[w].pt)) {
                        p = w;
                        w = verts[w].nxt;
                    }

                    std::cout << v << " -> " << p << std::endl;

                    double *ptW = verts[w].pt;
                    //double *ptP = verts[p].pt;

                    double cC = Cross(x, ptV, ptW),
                        cD = Cross(ptV, ptU, ptW);

                    std::cout << "cC " << cC << std::endl;
                    std::cout << "cD " << cD << std::endl;

                    if (cC < 0) {

                        if (cD < 0 || IsNear(ptU, ptW)) {
                            verts[vp.back()].nxt = p;

                            vp.push_back(p);
                        } else {
                            int _x = w;

                            for (;;) {
                                Vert &a = verts[_x],
                                    &b = verts[a.nxt];

                                double *ptA = a.pt,
                                    *ptB = b.pt;

                                std::cout << ">> " << a.id << ", " << b.id << std::endl;

                                std::shared_ptr<D> d(Intersect(x, verts[v].r, ptA, ptB));

                                if (d
                                    && (!IsFrontfaced(verts[v].r, ptA, ptB)
                                        || IsNear(ptA, ptV))) { // spezialfall (special:1, ind:1)
                                    std::cout << "x" << std::endl;

                                    if (d->t2 < E) {
                                        verts[vp.back()].nxt = _x;
                                        vp.push_back(_x);

                                    } else {
                                        Vert _v(x, d->s, ref, a.nxt);
                                        verts.push_back(_v);
                                        int k = verts.size()-1;

                                        verts[vp.back()].nxt = k;

                                        vp.push_back(k);

                                    }

                                    break;
                                } else {
                                    _x = a.nxt;
                                }

                            }
                        }

                    } else {
                        verts[vp.back()].nxt = p;

                        vp.push_back(p);
                    }

                }
            }
        }
    }

    res.push_back(poly[ind]);

    for (int _v : vp) {
        int id = verts[_v].id;
        res.push_back({verts[_v].pt, id != NO_USE ? poly[id].id : id});
    }

}

double GetArea (const PolyType &poly) {
    int num = poly.size();

    double sum = 0;

    for (int i = 0; i < num; i++) {
        const Point &a = poly[i],
            &b = poly[(i+1)%num];
        sum += a.x*b.y-b.x*a.y;
    }

    return sum;
}

/*
list([ list(map(float, p.split(','))) for p in 'm 26.402829,29.895027 -2.132521,24.374833 -3.073759,35.133226 22.541594,1.972134 76.814397,6.720388 1.86346,-21.299507 3.34282,-38.208551 -31.800976,-2.782225 -0.800142,-0.07 -7.246298,-0.633968 -2.155836,24.641314 -6.148254,-0.537902 -8.586643,-0.751234 -4.925747,-0.430947 1.112312,-12.713787 0.198,-2.263145 0.192176,-2.196583 0.326117,-3.727536 0.327231,-3.740264 z'[2:-2].split(' ') ])
*/

void Magic (const PolyType &poly, PolyType &res, int omit) {

    double area = GetArea(poly);

    std::set<int> found;

    int num = poly.size();

    std::map<Point, int> counts;

    for (auto &p : poly) {
        counts[p]++;
    }

    for (auto &f : counts) {
        std::cout << "XX " << f.first << " -> " << f.second << std::endl;
    }

    std::cout << "XX ids'=[";

    for (int i = 0; i < num; i++) {
        const Point &pt = poly[i];

        if (pt.id == omit) {
            continue;
        }

        PolyType _poly;

        std::copy_if(poly.begin(), poly.end(), std::back_inserter(_poly), [&pt, &found](const Point &p) {
            return p.id != pt.id && found.count(p.id) == 0;
        });

        double _area = GetArea(_poly);

        double per = std::abs(1-_area/area);

        if (per < 1e-4 && counts[poly[i]] == 1) {
            area = _area;
            found.insert(pt.id);

            std::cout << i << ", ";
        }

        //std::cout << "(" << i << ", " << per << "), ";
    }

    std::cout << "]" << std::endl;

    std::copy_if(poly.begin(), poly.end(), std::back_inserter(res), [&found](const Point &p) {
        return found.count(p.id) == 0;
    });

}

class Vert2 {
public:
    Vert2 (int _i, double _l) : i(_i), l(_l) {}
    double l;
    int i;
};

typedef std::vector<Vert2> VertsType2;

void Align (PolyType &poly, const Point &p) {
    VertsType2 verts;

    PolyType::iterator itr;

    for (itr = poly.begin(); itr != poly.end(); ++itr) {
        if (itr->id != p.id) {
            double v[] = {itr->x-p.x, itr->y-p.y};
            verts.push_back({static_cast<int>(itr-poly.begin()), Normalize(v)});
        }
    }

    std::sort(verts.begin(), verts.end(), [](const Vert2 &a, const Vert2 &b) {
        return b.l < a.l;
    });

    VertsType2::iterator itr2, itr3;

    for (itr2 = verts.begin(); itr2 != verts.end()-1; ++itr2) {
        double n[] = {p.y-poly[itr2->i].y, poly[itr2->i].x-p.x};
        Normalize(n);
        double d = n[0]*p.x+n[1]*p.y;

        for (itr3 = itr2+1; itr3 != verts.end(); ++itr3) {
            Point &q = poly[itr3->i];

            double e = d-n[0]*q.x-n[1]*q.y;

            if (std::abs(e) < 1e-3) {
                q.pt[0] += n[0]*e;
                q.pt[1] += n[1]*e;
            }

        }
    }
}

bool GetVisPoly_wrapper (PolyType &poly, PolyType &res, int ind) {
    int i = 0;
    for (auto& p : poly) {
        p.id = i++;
    }

    PolyType poly2, poly3, poly4;

    Point x(poly[ind]);

    Magic(poly, poly2, ind);

    Align(poly2, x);

    TrivialRm(poly2, ind, x).GetSimplified(poly3);

    Magic(poly3, poly4, ind);

    try {
        GetVisPoly(poly4, res);
    } catch (const vp_error &e) {
        std::cout << "Error: " << e.what() << std::endl;

        return false;
    }

    return true;

}