%% Clear and Close all

clear;
close all;

%% Define State

graph = {{2,3}, {4,5}, {6,7}, {8, 0}, {8,10}, {9,10}, {9, 0}, {10, 0}, {10,0}};                        % Topology
EAD = [3.66 2.44 2.88 1 1.2 1.6 1 1 1 0];                                                   % Expected Delay
index = 1;
initialState = [index, EAD(index)];                                                         % Initial State
State = struct ('Graph', graph, 'EAD', EAD, 'InitialState', initialState);                  % Structure of State
T      = 800;                                                                               % number of trials
E      = 2;                                                                                 % maximum number of steps in a trial
TSTEPS = T*E;                                                                               % total number of steps

%% VARIABLES 

s = State.EAD;
d = length(s);
v      = 0;                                                                                 % evaluation at time t
v_prev = 0;                                                                                 % evaluation at time t-1
adv    = 0;                                                                                 % TD error
a      = zeros(1,1);                                                                        % action units
na     = 2;                                                                                 % number of actions
pos    = zeros(d,1);                                                                        % current position 
gpos   = s(d);                                                                              % goal position
ppath  = zeros(d,E);                                                                        % history of positions in a trial
spath  = zeros(d,E);                                                                        % history of TD error in a trial

w_v    = -1 * s.^-1                                                                        % evaluation weights
w_v(10)  = 0;
w_v1    = zeros(1,d);
w_a    = zeros(1,2);                                                                        % action weights

%% PARAMETERS

g      = .99;                                                                               % discount factor of the current evaluation
lre    = .08;                                                                               % learning rate of the evaluator
lra    = .08;                                                                               % learning rate of the actor


%% Start of the simulation (loop through the trials).

for trials = 1:T
    %%   initialize current_evaluation, previous_evaluation, tderror to 0 set current_position to a random value != reward_position
    v        = 0;
    v_prev   = 0;
    a        = 0;
    pos_prev = 0;
    ppath     = ppath*0;
    spath     = spath*0;
    %% Set the  initial position index
    
    posidx = 1;                                                             % get a random value in the interval length of state.
%     while posidx == find(rew==1)                                          % Verify that posidx is not the goal position.
%         posidx = randi(d);                                                % In case it is in the reward position get another random value.
%     end
    
    n = graph{posidx}
    n1 = cell2mat(n)
    n_prev = n1;
    cs = [s(n1)];
    w_a = sgm(cs(1), cs(2));
    posidx_prev = posidx;

    %% Calculate the current position using the position index.
    pos = 1*( (1:d) == posidx )';                   % return a vector of ones
                                                    % and zeros (one := the value
                                                    % in the vector is equal to
                                                    % posidx, zero:= otherwise )
    
    
    ppath(:,1) = pos;                               % Store the initial position in ppath.
    
    %% At the beginning the previous position is set as the same as the initial position.
    pos_prev    = pos;

%     a = randsample(n1, 1);
%     p = s(a);
%     

    for t=2:E
        
        p = rand;
        x = max(w_a);
        if(p < x)
            posidx = n1(1);
        else
            posidx = n1(2);
        end
        
        pos = ( (1:d) == posidx )';
        ppath(:,t) = pos;
        
        v = w_v1*pos;                                  % current evaluation (weighted sum).
        [w_v1, tde] = update_critic(w_v1, v, v_prev, 1, lre, g);
        
        
        
        
        
%         if(s(a)< s(posidx) && s(a) < s(a+1))
%             posidx    = posidx + 1;
%         end
%         posidx    = max( 1 , min( d , posidx ) );   % Control if the index is out of the bounds.
%         pos       = ( (1:d) == posidx )';           % Calculate position on the state vector.
%         ppath(:,t) = pos;                           % Store current position in ppath.
%      
%         %% evaluation
%       
%         v = w_v*pos;                                % current evaluation (weighted sum).
% 
%         %% TD error
%         adv = rew(posidx) + g*v - v_prev;           % TD error.
%         spath(:,t) = adv*pos;
%         
%         %% Eval learning
%         
%         w_v = w_v  +    ...                         % Update the evaluator weights.
%         lre *        ...                            % learning rate
%         adv *        ...                            % TD error 
%         pos_prev';   ...                            % input at previous time
% 
%         %% Actor learning
%         
%         w_a = w_a +               ...               % Update the actor weights.
%         lra *                  ...                  % learning rate
%         adv *                    ...                % TD error
%         a*(1-a) *      ...                          % derivative of the output of the actor
%         pos_prev';             ...                  % input at previous time
% 
% 
%         v_prev    = v;                              % store evaluation 
%         pos_prev  = pos;                            % store position
%         
    end
    
    
    
end

function y = sgm(x1, x2)
y1 = exp (-x1)./(exp(-x1)+exp(-x2));
y2 = exp (-x2)./(exp(-x1)+exp(-x2));
y = [y1 y2];
end

function [utility, TD] = update_critic(utility, value, value_prev, reward, lr_eval,df)
utility = utility + lr_eval(reward + (df * value) - value_prev)
TD = reward + (df * value) - value_prev;

end

function y = sigmoid(x)
y = 1./(1+exp(-x));
end