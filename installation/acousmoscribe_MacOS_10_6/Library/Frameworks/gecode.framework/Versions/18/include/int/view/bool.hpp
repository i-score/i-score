/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
 *
 *  Copyright:
 *     Christian Schulte, 2002
 *
 *  Last modified:
 *     $Date: 2009-09-08 21:10:29 +0200 (Di, 08 Sep 2009) $ by $Author: schulte $
 *     $Revision: 9692 $
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

namespace Gecode {

  namespace Int {

    /*
     * Constructors and initialization
     *
     */
    forceinline
    BoolView::BoolView(void) {}
    forceinline
    BoolView::BoolView(const BoolVar& x)
      : VarViewBase<BoolVarImp>(x.var()) {}
    forceinline
    BoolView::BoolView(BoolVarImp* x)
      : VarViewBase<BoolVarImp>(x) {}

    /*
     * Value access
     *
     */
    forceinline BoolStatus
    BoolView::status(void) const {
      return varimp->status();
    }
    forceinline int
    BoolView::min(void) const {
      return varimp->min();
    }
    forceinline int
    BoolView::max(void) const {
      return varimp->max();
    }
    forceinline int
    BoolView::med(void) const {
      return varimp->med();
    }
    forceinline int
    BoolView::val(void) const {
      return varimp->val();
    }

    forceinline unsigned int
    BoolView::size(void) const {
      return varimp->size();
    }
    forceinline unsigned int
    BoolView::width(void) const {
      return varimp->width();
    }
    forceinline unsigned int
    BoolView::regret_min(void) const {
      return varimp->regret_min();
    }
    forceinline unsigned int
    BoolView::regret_max(void) const {
      return varimp->regret_max();
    }


    /*
     * Domain tests
     *
     */
    forceinline bool
    BoolView::range(void) const {
      return varimp->range();
    }
    forceinline bool
    BoolView::assigned(void) const {
      return varimp->assigned();
    }

    forceinline bool
    BoolView::in(int n) const {
      return varimp->in(n);
    }
    forceinline bool
    BoolView::in(double n) const {
      return varimp->in(n);
    }


    /*
     * Domain update by value
     *
     */
    forceinline ModEvent
    BoolView::lq(Space& home, int n) {
      return varimp->lq(home,n);
    }
    forceinline ModEvent
    BoolView::lq(Space& home, double n) {
      return varimp->lq(home,n);
    }

    forceinline ModEvent
    BoolView::le(Space& home, int n) {
      return varimp->lq(home,n-1);
    }
    forceinline ModEvent
    BoolView::le(Space& home, double n) {
      return lq(home,n-1.0);
    }

    forceinline ModEvent
    BoolView::gq(Space& home, int n) {
    return varimp->gq(home,n);
    }
    forceinline ModEvent
    BoolView::gq(Space& home, double n) {
      return varimp->gq(home,n);
    }

    forceinline ModEvent
    BoolView::gr(Space& home, int n) {
      return varimp->gq(home,n+1);
    }
    forceinline ModEvent
    BoolView::gr(Space& home, double n) {
      return gq(home,n+1.0);
    }

    forceinline ModEvent
    BoolView::nq(Space& home, int n) {
      return varimp->nq(home,n);
    }
    forceinline ModEvent
    BoolView::nq(Space& home, double n) {
      return varimp->nq(home,n);
    }

    forceinline ModEvent
    BoolView::eq(Space& home, int n) {
      return varimp->eq(home,n);
    }
    forceinline ModEvent
    BoolView::eq(Space& home, double n) {
      return varimp->eq(home,n);
    }


    /*
     * Iterator-based domain update
     *
     */
    template<class I>
    forceinline ModEvent
    BoolView::narrow_r(Space& home, I& i, bool depend) {
      Iter::Ranges::IsRangeIter<I>();
      return varimp->narrow_r(home,i,depend);
    }
    template<class I>
    forceinline ModEvent
    BoolView::inter_r(Space& home, I& i, bool depend) {
      Iter::Ranges::IsRangeIter<I>();
      return varimp->inter_r(home,i,depend);
    }
    template<class I>
    forceinline ModEvent
    BoolView::minus_r(Space& home, I& i, bool depend) {
      Iter::Ranges::IsRangeIter<I>();
      return varimp->minus_r(home,i,depend);
    }
    template<class I>
    forceinline ModEvent
    BoolView::narrow_v(Space& home, I& i, bool depend) {
      Iter::Values::IsValueIter<I>();
      return varimp->narrow_v(home,i,depend);
    }
    template<class I>
    forceinline ModEvent
    BoolView::inter_v(Space& home, I& i, bool depend) {
      Iter::Values::IsValueIter<I>();
      return varimp->inter_v(home,i,depend);
    }
    template<class I>
    forceinline ModEvent
    BoolView::minus_v(Space& home, I& i, bool depend) {
      Iter::Values::IsValueIter<I>();
      return varimp->minus_v(home,i,depend);
    }


    /*
     * Boolean domain tests
     *
     */
    forceinline bool
    BoolView::zero(void) const {
      return varimp->zero();
    }
    forceinline bool
    BoolView::one(void) const {
      return varimp->one();
    }
    forceinline bool
    BoolView::none(void) const {
      return varimp->none();
    }


    /*
     * Boolean assignment operations
     *
     */
    forceinline ModEvent
    BoolView::zero_none(Space& home) {
      return varimp->zero_none(home);
    }
    forceinline ModEvent
    BoolView::one_none(Space& home) {
      return varimp->one_none(home);
    }

    forceinline ModEvent
    BoolView::zero(Space& home) {
      return varimp->zero(home);
    }
    forceinline ModEvent
    BoolView::one(Space& home) {
      return varimp->one(home);
    }


    /*
     * Delta information for advisors
     *
     */
    forceinline ModEvent
    BoolView::modevent(const Delta& d) {
      return BoolVarImp::modevent(d);
    }
    forceinline int
    BoolView::min(const Delta& d) const {
      return BoolVarImp::min(d);
    }
    forceinline int
    BoolView::max(const Delta& d) const {
      return BoolVarImp::max(d);
    }
    forceinline bool
    BoolView::any(const Delta& d) const {
      return BoolVarImp::any(d);
    }
    forceinline bool
    BoolView::zero(const Delta& d) {
      return BoolVarImp::zero(d);
    }
    forceinline bool
    BoolView::one(const Delta& d) {
      return BoolVarImp::one(d);
    }



    forceinline ModEventDelta
    BoolView::med(ModEvent me) {
      return VarViewBase<BoolVarImp>::med(me);
    }


    /*
     * Cloning
     *
     */
    forceinline void
    BoolView::update(Space& home, bool share, BoolView& x) {
      varimp = x.varimp->copy(home,share);
    }

    /**
     * \brief %Range iterator for Boolean variable views
     * \ingroup TaskActorIntView
     */
    template<>
    class ViewRanges<BoolView> : public Iter::Ranges::Singleton {
    public:
      /// \name Constructors and initialization
      //@{
      /// Default constructor
      ViewRanges(void);
      /// Initialize with ranges for view \a x
      ViewRanges(const BoolView& x);
      /// Initialize with ranges for view \a x
      void init(const BoolView& x);
      //@}
    };

    forceinline
    ViewRanges<BoolView>::ViewRanges(void) {}

    forceinline
    ViewRanges<BoolView>::ViewRanges(const BoolView& x)
      : Iter::Ranges::Singleton(x.min(),x.max()) {}

    forceinline void
    ViewRanges<BoolView>::init(const BoolView& x) {
      Iter::Ranges::Singleton::init(x.min(),x.max());
    }

  }

}

// STATISTICS: int-var
