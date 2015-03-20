% Plot Precision-Recall curve

PR_files = dir(fullfile('Data\\result_PR', '*'));
PR_names = {PR_files.name};

id_figure = 0;
for idx=1:numel(PR_names)
	PR_name = PR_names{idx};
	if (strcmp(PR_name, '..') || strcmp(PR_name, '.') || strcmp(PR_name, 'desktop.ini'))
		continue;
    end
	
    id_figure = id_figure + 1;
    
	A = load(fullfile('Data\\result_PR', PR_name));
    recall = A(1:end, 1);
    precision = A(1:end, 2);

    fig = figure('visible', 'off');
	plot(recall, Fecision, 'Marker', '.');
    title(F_name);
    xlabel('Recall');
    ylabel('Precision');
    print(fig, fullfile('Data\\result_PR', strcat(F_name, '.png')), '-dpng');
end
