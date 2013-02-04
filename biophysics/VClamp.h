/* VClamp.h --- 
 * 
 * Filename: VClamp.h
 * Description: 
 * Author: 
 * Maintainer: 
 * Created: Fri Feb  1 19:22:19 2013 (+0530)
 * Version: 
 * Last-Updated: Mon Feb  4 19:00:09 2013 (+0530)
 *           By: subha
 *     Update #: 50
 * URL: 
 * Keywords: 
 * Compatibility: 
 * 
 */

/* Commentary: 
 * 
 * Class for implementing voltage clamp
 * 
 */

/* Change log:
 * 
 * 
 */

/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 */

/* Code: */

#ifndef _VCLAMP_H
#define _VCLAMP_H
namespace moose
{
    class VClamp
    {
  public:
        VClamp();
        ~VClamp();
        void setHoldingPotential(double v);
        double getHoldingPotential() const;
        void setVin(double v);
        void setGain(double value);
        double getGain() const;
        double getCurrent() const;
        void process(const Eref& e, ProcPtr p);
        void reinit(const Eref& e, ProcPtr p);
        
        static const Cinfo* initCinfo();

        // finfo used to send out injection current to compartment
        static SrcFinfo1< double >* currentOut();
        
  protected:
        double vIn_; // Membrane potential read from the compartment
        double holding_; // holding potential
        double current_; // Current generated by the clamp circuit
        double gain_; // Access resistance of the clamp
    };
}

#endif // _VCLAMP_H
/* VClamp.h ends here */
