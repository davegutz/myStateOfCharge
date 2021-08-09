// Copyright (C) 2019  - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Jun 22, 2019    DA Gutz     Created
//
function export_figs(figs, run_name)
    files_to_delete = listfiles('plots/'+run_name+'*.pdf');
    for i=1:size(files_to_delete, 1)
        mdelete(files_to_delete(i))
    end
    files_to_delete = listfiles('plots/raw/'+run_name+'*.png');
    for i=1:size(files_to_delete, 1)
        mdelete(files_to_delete(i))
    end
    for i = 1:length(figs)
        xs2png(figs(i), 'plots/raw/'+run_name+string(i)+'.png')
        xs2pdf(figs(i), 'plots/'+run_name+string(i)+'.pdf', 'landscape')
    end
    mdelete('plots/temp.pdf')
    res = dos('pdftk plots/'+run_name+'*.pdf output plots/temp.pdf')
    if ~res then
        messagebox('Failed to export_figs.  '+run_name+' may be open in a viewer or folder structure not created or pdftk not installed (restart of scilab required)', gettext("export_figs"), "Error", "modal");
    end
    mdelete('plots/'+run_name+'.pdf');
    res = dos('pdftk plots/temp.pdf cat 1-endwest output plots/'+run_name+'.pdf')
    if ~res then
        messagebox('Failed to export_figs.  '+run_name+' may be open in a viewer or folder structure not created or pdftk not installed (restart of scilab required)', gettext("export_figs"), "Error", "modal");
    end
    mdelete('plots/temp.pdf')
    for i = 1:length(figs)
        mdelete('plots/'+run_name+string(i)+'.pdf')
    end
endfunction
